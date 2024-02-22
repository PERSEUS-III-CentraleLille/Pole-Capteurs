#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>

// Fonction pour vérifier l'allocation de mémoire
void checkAllocation(void *ptr) {
    if (!ptr) {
        perror("Erreur d'allocation de mémoire");
        exit(EXIT_FAILURE);
    }
}

typedef struct {
    char **data;       // Ensemble de données
    int sizeLines;      // Nombre de lignes de données
    int sizeColumns;    // Nombre de colonnes de données
    int missingLines;   // Nombre de lignes incomplètes
} data_lines;

int set_l2cap_mtu(int s, uint16_t mtu) { // Fonction qui change le MTU d'un socket
    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
}

data_lines DataConvert(char *lien) { // Fonction de conversion des fichiers txt en char *
    FILE *fichier = fopen(lien, "r");
    if (fichier == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

    data_lines dataConverted;
    char **data = NULL;
    dataConverted.data = NULL;
    dataConverted.sizeColumns = dataConverted.sizeLines = dataConverted.missingLines = 0;

    int c1 = 0; // Compteur de lignes
    int c2 = 0; // Compteur de colonnes
    int c3 = 0; // Compteur annexe

    // Allocation mémoire initiale
    data = (char **)malloc(sizeof(char *));
    checkAllocation(data);

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);

        if (currentChar == '\n' || feof(fichier)) {
            c1++;

            if (c1 == 1) {
                c3 = c2; // Stockage du nombre de colonnes "normal" du fichier
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 == c3 + 1 || c2 == c3 + 2 || c2 == c3 + 3 || c2 == c3 - 1 || c2 == c3 - 2 || c2 == c3 - 3) {
                    c3 = c2; // Stockage du nouveau nombre "normal" de colonnes
                    dataConverted.sizeColumns = c3;
                }
                if (c2 != c3) {
                    printf("Il manque %d caractères dans la ligne %d du fichier %s\n", abs(c2 - c3), c1 - 1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 0; // Retour à la première colonne

            // Réallocation mémoire pour la nouvelle ligne
            data = (char **)realloc(data, (c1 + 1) * sizeof(char *));
            checkAllocation(data);

            data[c1] = (char *)malloc((c3 + 1) * sizeof(char)); // Vous devez allouer c3 + 1 pour le '\0'
            checkAllocation(data[c1]);
        } else {
            c2++;

            // Réallocation mémoire pour la nouvelle colonne
            data[c1] = (char *)realloc(data[c1], (c2 + 1) * sizeof(char));
            checkAllocation(data[c1]);

            data[c1][c2 - 1] = currentChar;
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = c1;

    fclose(fichier);

    return dataConverted;
}

int errorRate(data_lines data1, data_lines data2) { // Fonction calculant le taux de perte et d'erreur dans la transmission
    float nb_errors, nb_data = 0;
    double loss_rate, error_rate = 0.0;
    int minLines, maxColumns, deltaLines = 0;

    float Lines1 = data1.sizeLines;
    float mLines1 = data1.missingLines;
    float Lines2 = data2.sizeLines;
    float mLines2 = data2.missingLines;

    if (Lines1 >= Lines2) {
        loss_rate = (Lines1 - Lines2 + mLines1 + mLines2) / Lines1 * 100;
        minLines = Lines2;
        deltaLines = Lines1 - Lines2;
        maxColumns = data1.sizeColumns;
    } else {
        loss_rate = (Lines2 - Lines1 + mLines1 + mLines2) / Lines2 * 100;
        minLines = Lines1;
        deltaLines = Lines2 - Lines1;
        maxColumns = data1.sizeColumns;
    }

    printf("Taux de perte de %.10lf pourcents\n", loss_rate);

    int i, j = 0;
    for (i = 0; i < minLines; i++) {
        for (j = 0; j < maxColumns; j++) {
            nb_data++;
            if (data1.data[i][j] != data2.data[i][j]) {
                nb_errors++;
            }
        }
    }

    nb_errors += deltaLines * maxColumns;

    error_rate = nb_errors / nb_data * 100;
    printf("Taux d'erreur de %.10lf pourcents\n", error_rate);

    return 0;
}

int main(int argc, char **argv) { // Fonction de réception des données

// Commande pour exécuter bluetoothctl
    char *bluetoothctl_command = "bluetoothctl";

    // Ouvrir un tube pour envoyer des commandes à bluetoothctl
    FILE *pipe = popen(bluetoothctl_command, "w");

    if (!pipe) {
        perror("Erreur lors de l'ouverture du tube.");
        return 1;
    }

    // Envoyer les commandes à bluetoothctl
    fprintf(pipe, "power on\n");
    fprintf(pipe, "discoverable yes\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    
    
        // Commande pour activer le Bluetooth
    char *command = "sudo hciconfig hci0 up";

    // Exécution de la commande système
    int status = system(command);

    if (status == 0) {
        printf("Bluetooth activé avec succès.\n");
    } else {
        printf("Erreur lors de l'activation du Bluetooth.\n");
    }

    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    data_lines data;
    data.data = (char **)malloc(sizeof(char *));
    checkAllocation(data.data);
    data.data[0] = (char *)malloc(sizeof(char));
    checkAllocation(data.data[0]);
    data.data[0][0] = '\0';

    struct sched_param sched_p;
    sched_p.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) {
        perror("sched_setscheduler \n");
        exit(EXIT_FAILURE);
    }

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    checkAllocation(&s);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s, 1);

    if (s == -1) {
        perror("socket");
        exit(1);
    }

    int mtu_value = 15620;
    set_l2cap_mtu(s, mtu_value);

    data_lines initial_data;
    initial_data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");

    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char));
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char));

    FILE *fichier = NULL;
    fichier = fopen("test.txt", "w+");
    checkAllocation(fichier);
    FILE *resultat = NULL;
    resultat = fopen("result.txt", "w+");
    checkAllocation(resultat);

    if (fichier != NULL) {
        // On peut lire et écrire dans le fichier
    } else {
        printf("Impossible d'ouvrir le fichier test.txt");
    }

    if (resultat != NULL) {
        // On peut lire et écrire dans le fichier
    } else {
        printf("Impossible d'ouvrir le fichier resultat.txt");
    }

    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    int check = 1;
    int i, j, final_j = 0;
    long tempsboucle, temps_envoi = 0;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    while (check) {
        bytes_read = recv(client, buf, mtu_value, 0);
        if (bytes_read > 0) {
            if (strcmp(buf, "stop") == 0) {
                check = 0;
                gettimeofday(&end, NULL);
                temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
                printf("\nTemps de transmission : %ld micro secondes\n\n", temps_envoi);
                fprintf(resultat, "%ld ms\n", temps_envoi);
            } else {
                strcpy(test, buf);
                if (fichier != NULL) {
                    fprintf(fichier, test);
                    int l = 0;
                    for (l = 0; l < bytes_read; l++) {
                        if (test[l] == '@') {
                            data.data[i][j] = '\n';
                            i++;
                            if (j > final_j && i > 1) {
                                final_j = j;
                            }
                            j = 0;
                            data.data = (char **)realloc(data.data, (i + 1) * sizeof(char *));
                            checkAllocation(data.data);
                            data.data[i] = (char *)malloc(sizeof(char));
                            checkAllocation(data.data[i]);
                        } else if (test[l] == '\0') {
                            data.data[i][j] = '\0';
                            l = bytes_read;
                            data.sizeLines = i;
                            data.sizeColumns = final_j;
                            data.missingLines = 0;
                        } else {
                            data.data[i][j] = test[l];
                            j++;
                            data.data[i] = (char *)realloc(data.data[i], (j + 1) * sizeof(char));
                            checkAllocation(data.data[i]);
                        }
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        }
    }

    fclose(resultat);
    fclose(fichier);

    printf("Taille du fichier initial : %d x %d\n", initial_data.sizeLines, initial_data.sizeColumns);
    printf("Taille du fichier final : %d x %d\n\n", data.sizeLines, data.sizeColumns);

    errorRate(initial_data, data);

    close(client);
    close(s);

    for (i = 0; i < data.sizeLines; i++) {
        free(data.data[i]);
    }
    free(data.data);

    return 0;
}


/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>

// Fonction pour vérifier l'allocation de mémoire
void checkAllocation(void *ptr) {
    if (!ptr) {
        perror("Erreur d'allocation de mémoire");
        exit(EXIT_FAILURE);
    }
}


typedef struct {
    char **data;       // Ensemble de données
    int sizeLines;      // Nombre de lignes de données
    int sizeColumns;    // Nombre de colonnes de données
    int missingLines;   // Nombre de lignes incomplètes
} data_lines;

int set_l2cap_mtu(int s, uint16_t mtu) { // Fonction qui change le MTU d'un socket

    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
}

data_lines DataConvert(char * lien) { // Fonction de conversion des fichiers txt en char *


    // Ouverture du fichier
    FILE *fichier = fopen(lien, "r");

    // Vérification de l'ouverture du fichier
    if (fichier != NULL) {
        //pas de problème
    }
    else{
        printf("Erreur lors de l'ouverture du fichier");
    }

    // Initialisations
    data_lines dataConverted;
    char **data = NULL;
    dataConverted.data = NULL;
    dataConverted.sizeColumns = dataConverted.sizeLines = dataConverted.missingLines = 0;

    int c1 = 0; // Compteur de lignes
    int c2 = 0; // Compteur de colonnes
    int c3 = 0; // Compteur annexe

    //Allocation mémoire initiale
    data = (char **) malloc (c1 * sizeof(char *));
    checkAllocation(data);
    
    if (data == NULL) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }

    data[c1 - 1] = (char *)malloc(c2 * sizeof(char));
    checkAllocation(data[c1 - 1]);
    if (data[c1 - 1] == NULL) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }
    while (!feof(fichier)) { // Tant qu'on n'est pas arrivés à la fin du fichier

        //Récupération du caractère lu
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentChar == '\n' || feof(fichier)) { // Si on a un saut de ligne ou la fin du fichier
            c1++;
            if (c1 == 2) {
                c3 = c2; // Stockage du nombre de colonnes "normal" du fichier
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 == c3 + 1 || c2 == c3 + 2 || c2 == c3 + 3 || c2 == c3 - 1 || c2 == c3 - 2 || c2 == c3 - 3) {
                    // Présence (ou non) des - dans les données
                    c3 = c2; // Stockage du nouveau nombre "normal" de colonnes
                    dataConverted.sizeColumns = c3;
                }
                if (c2 != c3) {
                    printf("Il manque %d caractères dans la ligne %d du fichier %s\n", abs(c2 - c3), c1-1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 1; // Retour à la première colonne

            // Réallocation mémoire pour la nouvelle ligne
            data = (char **)realloc(data, (c1 + 1) * sizeof(char *));
            checkAllocation(data);
            if (data == NULL) {
          perror("Erreur réallocation mémoire");
          exit(EXIT_FAILURE);
          }
            data[c1] = (char *)malloc((c3 + 1) * sizeof(char)); // Vous devez allouer c3+1 pour le '\0'
            checkAllocation(data[c1]);

            if (!data[c1]) {
                perror("Erreur lors de l'allocation de mémoire");
                exit(EXIT_FAILURE);
            }

          
        } else {
            c2++;
            // Réallocation mémoire pour la nouvelle colonne
            data[c1 - 1] = (char *)realloc(data[c1 - 1], c2 * sizeof(char));
             if (data == NULL) {
          perror("Erreur réallocation mémoire");
          exit(EXIT_FAILURE);
          }
            data[c1 - 1][c2] = currentChar;
            
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = c1;

    fclose(fichier);

    return dataConverted;
}
   


int errorRate(data_lines data1, data_lines data2) { // Fonction calculant le taux de perte et d'erreur dans la transmission

    // Initialisations
    float nb_errors, nb_data = 0;
    double loss_rate, error_rate = 0.0;
    int minLines, maxColumns, deltaLines = 0;

    // Transformations en float (pour les divisions)
    float Lines1 = data1.sizeLines;
    float mLines1 = data1.missingLines;
    float Lines2 = data2.sizeLines;
    float mLines2 = data2.missingLines;

    // Calcul du taux de perte
    if (Lines1 >= Lines2) { // Le fichier 1 est plus long que le 2 (ou de même taille)
        loss_rate = (Lines1 - Lines2 + mLines1 + mLines2) / Lines1 * 100;
        minLines = Lines2;
        deltaLines = Lines1 - Lines2;
        maxColumns = data1.sizeColumns;
    }
    else { // Le fichier 2 est plus long que le 1
        loss_rate = (Lines2 - Lines1 + mLines1 + mLines2) / Lines2 * 100;
        minLines = Lines1;
        deltaLines = Lines2 - Lines1;
        maxColumns = data1.sizeColumns;
    }

    // Calcul du taux de perte de données
    printf("Taux de perte de %.10lf pourcents\n", loss_rate);

    // Initialisations
    int i, j = 0;

    // Calcul du taux d'erreur
    for (i = 0; i < minLines; i++) { // On doit prendre la plus petite longueur de fichier pour éviter de parcourir un fichier fini
        for (j = 0; j < maxColumns; j++) {
            nb_data++;
            if (data1.data[i][j] != data2.data[i][j]) { // Si les char sont différents
                nb_errors++;
            }
        }
    }

    // Ajout des lignes manquantes (car on a pris la plus petite longueur de fichier possible)
    nb_errors += deltaLines * maxColumns;

    // Calcul du taux d'erreur dans les données
    error_rate = nb_errors / nb_data * 100;
    printf("Taux d'erreur de %.10lf pourcents\n", error_rate);

    return 0;
}

int main(int argc, char **argv) { // Fonction de réception des données

    // Création du socket de réception et initialisations des données
    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0}; // struct de socket
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    // Création du data_lines final
    data_lines data;
    data.data = (char **)malloc(sizeof(char *));
    checkAllocation(data.data);
    data.data[0] = (char *)malloc(sizeof(char));
    checkAllocation(data.data[0]);
    data.data[0][0] = '\0';

    // Définition de la priorité du script en priorité temps réel
    struct sched_param sched_p; // Création d'une structure d'ordonnancement temps réel pour le programme
    sched_p.sched_priority = 50; // Affectation d'une priorité temps réel entre 0 et 99
    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) { // Affectation d'un ordonancement Round-robin avec le paramètre de priorité défini précédemment si l'opération se passe sans erreur
        perror("sched_setscheduler \n"); // Sinon le programme se termine via la fonction perror()
        exit(EXIT_FAILURE);
    }

    // Allocation du socket
    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    checkAllocation(&s);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s, 1);

	if (s == -1) { //vérification socket code de Bourdeaud'hui
		perror("socket"); 
		exit(1);
	}
    // Modification du MTU
    int mtu_value = 15620; // Valeur modifiée par le fichier python
    set_l2cap_mtu(s, mtu_value);

    // Conversion du fichier initial
    data_lines initial_data;
    initial_data = DataConvert("/home/PERSEUS2023/Pole-Capteurs/server/Numerical_Results_capteur.txt");

    // Initialisations de réception
    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char)); // Initialisation du buffer avec des zéros
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char)); // Initialisation de test avec des zéros

    // Ouverture du fichier de résultat et du fichier de réception des données
    FILE *fichier = NULL;
    fichier = fopen("test.txt", "w+");
    checkAllocation(fichier);
    FILE *resultat = NULL;
    resultat = fopen("result.txt", "w+");
    checkAllocation(resultat);
//Vérification ouverture des fichiers test et résultat
  if (fichier != NULL)
    {
        // On peut lire et écrire dans le fichier
    }
    else
    {
        // On affiche un message d'erreur si on veut
        printf("Impossible d'ouvrir le fichier test.txt");
    }
  if (resultat != NULL)
    {
        // On peut lire et écrire dans le fichier
    }
    else
    {
        // On affiche un message d'erreur si on veut
        printf("Impossible d'ouvrir le fichier resultat.txt");
    }

    // Acceptation de la connexion entre les Raspberry
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    // Réception de données
    int check = 1;
    int i, j, final_j = 0;
    long tempsboucle, temps_envoi = 0;
    struct timeval start, end; // Initialisation de variables de temps

    gettimeofday(&start, NULL); // Initialisation du temps de fin
    while (check) {
        bytes_read = recv(client, buf, mtu_value, 0); // Réception des données du client
        if (bytes_read > 0) {
            if (strcmp(buf, "stop") == 0) { // Quand on est arrivés à la fin de l'envoi
                check = 0;
                gettimeofday(&end, NULL); // Initialisation du temps de fin
                temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
                printf("\nTemps de transmission : %ld micro secondes\n\n", temps_envoi);
                // Temps total de transmission
                fprintf(resultat, "%ld ms\n", temps_envoi);
            }
            else {
                strcpy(test, buf); // Copie du buffer vers la mémoire "test"
                if (fichier != NULL) {
                    fprintf(fichier, test); // On écrit dans le fichier la mémoire "test"
                    int l = 0;
                    for (l = 0; l < bytes_read; l++) { // Pour chaque caractère lu
                        if (test[l] == '@') {          // Si on a un saut de ligne
                            data.data[i][j] = '\n';
                            i++;
                            if (j > final_j && i > 1) {
                                final_j = j; // Afin de déterminer le nombre de colonnes maximal
                            }
                            j = 0;
                            data.data = (char **)realloc(data.data, (i + 1) * sizeof(char *));
                            checkAllocation(data.data);
                            // Réallocation d'une nouvelle ligne
                            data.data[i] = (char *)malloc(sizeof(char));
                            checkAllocation(data.data[i]);
                        }
                        else if (test[l] == '\0') { // Si on est à la fin des données
                            data.data[i][j] = '\0';
                            l = bytes_read;
                            data.sizeLines = i; // Initialisation des derniers paramètres de data_lines
                            data.sizeColumns = final_j;
                            data.missingLines = 0; // Il faudrait rajouter une vérification des lignes incomplètes
                        }
                        else {
                            data.data[i][j] = test[l]; // Ajout au data
                            j++;
                            data.data[i] = (char *)realloc(data.data[i], (j + 1) * sizeof(char));
                            checkAllocation(data.data[i]);
                            // Réallocation mémoire
                        }
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        }
    }

    // Fermeture des

    // Fermeture des fichiers
    fclose(resultat);
    fclose(fichier);

    // Taille des data_lines
    printf("Taille du fichier initial : %d x %d\n", initial_data.sizeLines, initial_data.sizeColumns);
    printf("Taille du fichier final : %d x %d\n\n", data.sizeLines, data.sizeColumns);

    // Calcul du taux de perte et d'erreur
    errorRate(initial_data, data);

    // Fermeture des sockets
    close(client);
    close(s);

    // Libération de la mémoire
    for (i = 0; i < data.sizeLines; i++) {
        free(data.data[i]);
    }
    free(data.data);

    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>

typedef struct {
    char **data;       // Ensemble de données
    int sizeLines;      // Nombre de lignes de données
    int sizeColumns;    // Nombre de colonnes de données
    int missingLines;   // Nombre de lignes incomplètes
} data_lines;

int set_l2cap_mtu(int s, uint16_t mtu) { // Fonction qui change le MTU d'un socket

    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
}

data_lines DataConvert(char * lien) { // Fonction de conversion des fichiers txt en char *


    // Ouverture du fichier
    FILE *fichier = fopen(lien, "r");

    // Vérification de l'ouverture du fichier
    if (!fichier) {
        perror("Erreur lors de l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

    // Initialisations
    data_lines dataConverted;
    char **data = NULL;
    dataConverted.data = NULL;
    dataConverted.sizeColumns = dataConverted.sizeLines = dataConverted.missingLines = 0;

    int c1 = 0; // Compteur de lignes
    int c2 = 0; // Compteur de colonnes
    int c3 = 0; // Compteur annexe

    while (!feof(fichier)) { // Tant qu'on n'est pas arrivés à la fin du fichier

        // Caractère lu actuellement
        char currentChar = fgetc(fichier);

        if (currentChar == '\n' || feof(fichier)) { // Si on a un saut de ligne ou la fin du fichier

            if (c1 == 0) {
                c3 = c2; // Stockage du nombre de colonnes "normal" du fichier
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 == c3 + 1 || c2 == c3 + 2 || c2 == c3 + 3 || c2 == c3 - 1 || c2 == c3 - 2 || c2 == c3 - 3) {
                    // Présence (ou non) des - dans les données
                    c3 = c2; // Stockage du nouveau nombre "normal" de colonnes
                    dataConverted.sizeColumns = c3;
                }
                if (c2 != c3) {
                    printf("Il manque %d caractères dans la ligne %d du fichier %s\n", abs(c2 - c3), c1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 0; // Retour à la première colonne

            // Réallocation mémoire pour la nouvelle ligne
            data = (char **)realloc(data, (c1 + 1) * sizeof(char *));
            data[c1] = (char *)malloc((c3 + 1) * sizeof(char)); // Vous devez allouer c3+1 pour le '\0'

            if (!data[c1]) {
                perror("Erreur lors de l'allocation de mémoire");
                exit(EXIT_FAILURE);
            }

            ++c1;
        } else {
            // Réallocation mémoire pour la nouvelle colonne
            data[c1 - 1] = (char *)realloc(data[c1 - 1], (c2 + 1) * sizeof(char));
            data[c1 - 1][c2] = currentChar;
            ++c2;
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = c1;

    fclose(fichier);

    return dataConverted;
}
   

// Fonction pour vérifier l'allocation de mémoire
void checkAllocation(void *ptr) {
    if (!ptr) {
        perror("Erreur d'allocation de mémoire");
        exit(EXIT_FAILURE);
    }
}

int errorRate(data_lines data1, data_lines data2) { // Fonction calculant le taux de perte et d'erreur dans la transmission

    // Initialisations
    float nb_errors, nb_data = 0;
    double loss_rate, error_rate = 0.0;
    int minLines, maxColumns, deltaLines = 0;

    // Transformations en float (pour les divisions)
    float Lines1 = data1.sizeLines;
    float mLines1 = data1.missingLines;
    float Lines2 = data2.sizeLines;
    float mLines2 = data2.missingLines;

    // Calcul du taux de perte
    if (Lines1 >= Lines2) { // Le fichier 1 est plus long que le 2 (ou de même taille)
        loss_rate = (Lines1 - Lines2 + mLines1 + mLines2) / Lines1 * 100;
        minLines = Lines2;
        deltaLines = Lines1 - Lines2;
        maxColumns = data1.sizeColumns;
    }
    else { // Le fichier 2 est plus long que le 1
        loss_rate = (Lines2 - Lines1 + mLines1 + mLines2) / Lines2 * 100;
        minLines = Lines1;
        deltaLines = Lines2 - Lines1;
        maxColumns = data1.sizeColumns;
    }

    // Calcul du taux de perte de données
    printf("Taux de perte de %.10lf pourcents\n", loss_rate);

    // Initialisations
    int i, j = 0;

    // Calcul du taux d'erreur
    for (i = 0; i < minLines; i++) { // On doit prendre la plus petite longueur de fichier pour éviter de parcourir un fichier fini
        for (j = 0; j < maxColumns; j++) {
            nb_data++;
            if (data1.data[i][j] != data2.data[i][j]) { // Si les char sont différents
                nb_errors++;
            }
        }
    }

    // Ajout des lignes manquantes (car on a pris la plus petite longueur de fichier possible)
    nb_errors += deltaLines * maxColumns;

    // Calcul du taux d'erreur dans les données
    error_rate = nb_errors / nb_data * 100;
    printf("Taux d'erreur de %.10lf pourcents\n", error_rate);

    return 0;
}

int main(int argc, char **argv) { // Fonction de réception des données

    // Création du socket de réception et initialisations des données
    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0}; // struct de socket
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    // Création du data_lines final
    data_lines data;
    data.data = (char **)malloc(sizeof(char *));
    checkAllocation(data.data);
    data.data[0] = (char *)malloc(sizeof(char));
    checkAllocation(data.data[0]);
    data.data[0][0] = '\0';

    // Définition de la priorité du script en priorité temps réel
    struct sched_param sched_p; // Création d'une structure d'ordonnancement temps réel pour le programme
    sched_p.sched_priority = 50; // Affectation d'une priorité temps réel entre 0 et 99
    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) { // Affectation d'un ordonancement Round-robin avec le paramètre de priorité défini précédemment si l'opération se passe sans erreur
        perror("sched_setscheduler \n"); // Sinon le programme se termine via la fonction perror()
        exit(EXIT_FAILURE);
    }

    // Allocation du socket
    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    checkAllocation(&s);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s, 1);

    // Modification du MTU
    int mtu_value = 15620; // Valeur modifiée par le fichier python
    set_l2cap_mtu(s, mtu_value);

    // Conversion du fichier initial
    data_lines initial_data;
    initial_data = DataConvert("/home/PERSEUS2023/Pole-Capteurs/server/Numerical_Results_capteur.txt");

    // Initialisations de réception
    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char)); // Initialisation du buffer avec des zéros
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char)); // Initialisation de test avec des zéros

    // Ouverture du fichier de résultat et du fichier de réception des données
    FILE *fichier = NULL;
    fichier = fopen("test.txt", "w+");
    checkAllocation(fichier);
    FILE *resultat = NULL;
    resultat = fopen("result.txt", "w+");
    checkAllocation(resultat);

    // Acceptation de la connexion entre les Raspberry
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    // Réception de données
    int check = 1;
    int i, j, final_j = 0;
    long tempsboucle, temps_envoi = 0;
    struct timeval start, end; // Initialisation de variables de temps

    gettimeofday(&start, NULL); // Initialisation du temps de fin
    while (check) {
        bytes_read = recv(client, buf, mtu_value, 0); // Réception des données du client
        if (bytes_read > 0) {
            if (strcmp(buf, "stop") == 0) { // Quand on est arrivés à la fin de l'envoi
                check = 0;
                gettimeofday(&end, NULL); // Initialisation du temps de fin
                temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
                printf("\nTemps de transmission : %ld micro secondes\n\n", temps_envoi);
                // Temps total de transmission
                fprintf(resultat, "%ld ms\n", temps_envoi);
            }
            else {
                strcpy(test, buf); // Copie du buffer vers la mémoire "test"
                if (fichier != NULL) {
                    fprintf(fichier, test); // On écrit dans le fichier la mémoire "test"
                    int l = 0;
                    for (l = 0; l < bytes_read; l++) { // Pour chaque caractère lu
                        if (test[l] == '@') {          // Si on a un saut de ligne
                            data.data[i][j] = '\n';
                            i++;
                            if (j > final_j && i > 1) {
                                final_j = j; // Afin de déterminer le nombre de colonnes maximal
                            }
                            j = 0;
                            data.data = (char **)realloc(data.data, (i + 1) * sizeof(char *));
                            checkAllocation(data.data);
                            // Réallocation d'une nouvelle ligne
                            data.data[i] = (char *)malloc(sizeof(char));
                            checkAllocation(data.data[i]);
                        }
                        else if (test[l] == '\0') { // Si on est à la fin des données
                            data.data[i][j] = '\0';
                            l = bytes_read;
                            data.sizeLines = i; // Initialisation des derniers paramètres de data_lines
                            data.sizeColumns = final_j;
                            data.missingLines = 0; // Il faudrait rajouter une vérification des lignes incomplètes
                        }
                        else {
                            data.data[i][j] = test[l]; // Ajout au data
                            j++;
                            data.data[i] = (char *)realloc(data.data[i], (j + 1) * sizeof(char));
                            checkAllocation(data.data[i]);
                            // Réallocation mémoire
                        }
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        }
    }

    // Fermeture des

    // Fermeture des fichiers
    fclose(resultat);
    fclose(fichier);

    // Taille des data_lines
    printf("Taille du fichier initial : %d x %d\n", initial_data.sizeLines, initial_data.sizeColumns);
    printf("Taille du fichier final : %d x %d\n\n", data.sizeLines, data.sizeColumns);

    // Calcul du taux de perte et d'erreur
    errorRate(initial_data, data);

    // Fermeture des sockets
    close(client);
    close(s);

    // Libération de la mémoire
    for (i = 0; i < data.sizeLines; i++) {
        free(data.data[i]);
    }
    free(data.data);

    return 0;
}




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>

// Function to check file opening
FILE *openFile(const char *filename, const char *mode) {
    FILE *file = fopen(filename, mode);
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    return file;
}

// Function for memory allocation with error checking
void *safeMalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Function to free memory allocated for data
void freeData(char **data, int size) {
    for (int i = 0; i < size; i++) {
        free(data[i]);
    }
    free(data);
}

// Function to set L2CAP MTU
int setL2capMtu(int socket, uint16_t mtu) {
    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(socket, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(socket, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
}

// Data structure for storing converted data
typedef struct {
    char **data;
    int sizeLines;
    int sizeColumns;
    int missingLines;
} data_lines;

// Function to convert data from a file to a data_lines structure
data_lines dataConvert(const char *filename) {
    FILE *file = openFile(filename, "r");

    data_lines dataConverted;
    dataConverted.data = NULL;
    dataConverted.sizeColumns = dataConverted.sizeLines = dataConverted.missingLines = 0;

    int currentLines = 1, currentColumns = 1, normalColumns = 0;
    char currentChar;

    char **data = (char **)safeMalloc(currentLines * sizeof(char *));
    data[currentLines - 1] = (char *)safeMalloc(currentColumns * sizeof(char));

    while (!feof(file)) {
        currentChar = fgetc(file);
        int currentInt = currentChar;

        if (currentInt == 10) {  // If a newline character is encountered
            currentLines++;

            if (currentLines == 2) {
                normalColumns = currentColumns;
                dataConverted.sizeColumns = normalColumns;
            } else {
                if (currentColumns == normalColumns + 1 || currentColumns == normalColumns + 2 ||
                    currentColumns == normalColumns + 3 || currentColumns == normalColumns - 1 ||
                    currentColumns == normalColumns - 2 || currentColumns == normalColumns - 3) {
                    normalColumns = currentColumns;
                    dataConverted.sizeColumns = normalColumns;
                }
                if (currentColumns != normalColumns) {
                    printf("Missing %d characters in line %d of the file %s\n", abs(currentColumns - normalColumns),
                           currentLines - 1, filename);
                    dataConverted.missingLines++;
                }
            }
            currentColumns = 1;

            data = (char **)realloc(data, currentLines * sizeof(char *));
            data[currentLines - 1] = (char *)safeMalloc(currentColumns * sizeof(char));
        } else {
            currentColumns++;

            data[currentLines - 1] = (char *)realloc(data[currentLines - 1], currentColumns * sizeof(char));
            data[currentLines - 1][currentColumns - 2] = currentChar;
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = currentLines;

    fclose(file);

    return dataConverted;
}

// Function to calculate error rate between two data_lines structures
int errorRate(data_lines data1, data_lines data2) {
    float numErrors = 0, numData = 0;
    double lossRate, errorRate = 0.0;
    int minLines, maxColumns, deltaLines = 0;

    float lines1 = data1.sizeLines;
    float missingLines1 = data1.missingLines;
    float lines2 = data2.sizeLines;
    float missingLines2 = data2.missingLines;

    if (lines1 >= lines2) {
        lossRate = (lines1 - lines2 + missingLines1 + missingLines2) / lines1 * 100;
        minLines = lines2;
        deltaLines = lines1 - lines2;
        maxColumns = data1.sizeColumns;
    } else {
        lossRate = (lines2 - lines1 + missingLines1 + missingLines2) / lines2 * 100;
        minLines = lines1;
        deltaLines = lines2 - lines1;
        maxColumns = data1.sizeColumns;
    }

    printf("Loss rate: %.10lf percent\n", lossRate);

    int i, j = 0;

    for (i = 0; i < minLines; i++) {
        for (j = 0; j < maxColumns; j++) {
            numData++;
            if (data1.data[i][j] != data2.data[i][j]) {
                numErrors++;
            }
        }
    }

    numErrors += deltaLines * maxColumns;

    errorRate = numErrors / numData * 100;
    printf("Error rate: %.10lf percent\n", errorRate);

    return 0;
}

int main(int argc, char **argv) {
    // Create sockets and initialize data
    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int serverSocket = 0, clientSocket, bytesRead;
    unsigned int opt = sizeof(rem_addr);

    data_lines receivedData;
    receivedData.data = (char **)safeMalloc(sizeof(char *));
    receivedData.data[0] = (char *)safeMalloc(sizeof(char));
    receivedData.data[0][0] = '\0';

    struct sched_param schedParams;
    schedParams.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_RR, &schedParams) == -1) {
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }

    serverSocket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(serverSocket, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(serverSocket, 1);

    if (serverSocket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int mtuValue = 15620;
    setL2capMtu(serverSocket, mtuValue);

    data_lines initialData = dataConvert("/home/PERSEUS2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");

    char buf[mtuValue];
    memset(buf, 0, mtuValue * sizeof(char));
    char test[mtuValue];
    memset(test, 0, mtuValue * sizeof(char));

    FILE *file = openFile("test.txt", "w+");
    FILE *resultFile = openFile("result.txt", "w+");

    if (file == NULL || resultFile == NULL) {
        perror("Error opening files");
        exit(EXIT_FAILURE);
    }

    clientSocket = accept(serverSocket, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "Accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    int check = 1;
    int i, j, finalJ = 0;
    long loopTime, sendTime = 0;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    while (check) {
        bytesRead = recv(clientSocket, buf, mtuValue, 0);
        if (bytesRead > 0) {
            if (strcmp(buf, "stop") == 0) {
                check = 0;
                gettimeofday(&end, NULL);
                sendTime = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
                printf("\nTransmission time: %ld microseconds\n\n", sendTime);
                fprintf(resultFile, "%ld ms\n", sendTime);
            } else {
                strcpy(test, buf);
                if (file != NULL) {
                    fprintf(file, test);
                    int l = 0;
                    for (l = 0; l < bytesRead; l++) {
                        if (test[l] == '@') {
                            receivedData.data[i][j] = '\n';
                            i++;
                            if (j > finalJ && i > 1) {
                                finalJ = j;
                            }
                            j = 0;
                            receivedData.data = (char **)realloc(receivedData.data, (i + 1) * sizeof(char *));
                            receivedData.data[i] = (char *)safeMalloc(sizeof(char));
                        } else if (test[l] == '\0') {
                            receivedData.data[i][j] = '\0';
                            l = bytesRead;
                            receivedData.sizeLines = i;
                            receivedData.sizeColumns = finalJ;
                            receivedData.missingLines = 0;
                        } else {
                            receivedData.data[i][j] = test[l];
                            j++;
                            receivedData.data[i] = (char *)realloc(receivedData.data[i], (j + 1) * sizeof(char));
                        }
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        }
    }

    fclose(resultFile);
    fclose(file);

    printf("Initial file size: %d x %d\n", initialData.sizeLines, initialData.sizeColumns);
    printf("Final file size: %d x %d\n\n", receivedData.sizeLines, receivedData.sizeColumns);

    errorRate(initialData, receivedData);

    close(clientSocket);
    close(serverSocket);

    // Free allocated memory for data
    freeData(initialData.data, initialData.sizeLines);
    freeData(receivedData.data, receivedData.sizeLines);

    return 0;
}
*/
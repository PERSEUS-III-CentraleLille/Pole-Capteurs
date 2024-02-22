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
    char **data;
    int sizeLines;
    int sizeColumns;
    int missingLines;
} data_lines;

int set_l2cap_mtu(int s, uint16_t mtu) {
    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
}

data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");
    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier");
        exit(EXIT_FAILURE);
    }

    data_lines dataConverted;
    dataConverted.data = NULL;
    dataConverted.sizeColumns = dataConverted.sizeLines = dataConverted.missingLines = 0;

    size_t bufferSize = 0;
    char *lineBuffer = NULL;

    while (getline(&lineBuffer, &bufferSize, fichier) != -1) {
        int len = strlen(lineBuffer);
        if (lineBuffer[len - 1] == '\n') {
            lineBuffer[len - 1] = '\0';
        }

        char **tempData = realloc(dataConverted.data, (dataConverted.sizeLines + 1) * sizeof(char *));
        if (tempData == NULL) {
            perror("Erreur r�allocation m�moire");
            exit(EXIT_FAILURE);
        }

        dataConverted.data = tempData;
        dataConverted.data[dataConverted.sizeLines] = strdup(lineBuffer);

        dataConverted.sizeLines++;
        dataConverted.sizeColumns = strlen(lineBuffer);

        free(lineBuffer);
        lineBuffer = NULL;
        bufferSize = 0;
    }

    fclose(fichier);
    return dataConverted;
}

void freeData(data_lines *data) {
    for (int i = 0; i < data->sizeLines; i++) {
        free(data->data[i]);
    }
    free(data->data);
}

int errorRate(data_lines data1, data_lines data2) {
    float nb_errors = 0, nb_data = 0;
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

    int i, j;
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

int main() {
    char *bluetoothctl_command = "bluetoothctl";
    FILE *pipe = popen(bluetoothctl_command, "w");

    if (!pipe) {
        perror("Erreur lors de l'ouverture du tube.");
        return 1;
    }

    fprintf(pipe, "power on\n");
    fprintf(pipe, "discoverable yes\n");
    fprintf(pipe, "exit\n");
    fclose(pipe);

    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    data_lines data;
    data.data = NULL;
    data.sizeColumns = data.sizeLines = data.missingLines = 0;

    struct sched_param sched_p;
    sched_p.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) {
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s, 1);

    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int mtu_value = 15620;
    set_l2cap_mtu(s, mtu_value);

    data_lines initial_data = DataConvert("/home/PERSEUS2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");

    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char));
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char));

    char *currentLineBuffer = NULL;
    size_t currentBufferSize = 0;

    FILE *fichier = fopen("test.txt", "w+");
    FILE *resultat = fopen("result.txt", "w+");

    if (fichier == NULL || resultat == NULL) {
        perror("Impossible d'ouvrir le fichier");
        return EXIT_FAILURE;
    }

    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    int check = 1;
    int i = 0, j = 0, final_j = 0;
    long tempsboucle, temps_envoi = 0;
    struct timeval start, end;

    gettimeofday(&start, NULL);
     bytes_read = recv(client, buf, mtu_value, 0);


if (bytes_read > 0) {
    // Le code de traitement lorsque des donn�es sont lues avec succ�s
    printf("Nombre d'octets lus : %d\n", bytes_read);
} else if (bytes_read == 0) {
    // Le client a ferm� la connexion
    printf("Le client a ferm� la connexion.\n");
    }
    while (check) {
   
    if (bytes_read > 0) {
        if (strcmp(buf, "stop") == 0) {
            check = 0;
            gettimeofday(&end, NULL);
            temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
            printf("\nTemps de transmission : %ld micro secondes\n\n", temps_envoi);
            fprintf(resultat, "%ld ms\n", temps_envoi);
        } else {
            for (int l = 0; l < bytes_read; l++) {
                if (test[l] == '@') {
                    if (i > 0) {
                        currentLineBuffer[j] = '\0';
                        data.data[i] = strdup(currentLineBuffer);
                        if (data.data[i] == NULL) {
                            perror("Erreur d'allocation m�moire");
                            exit(EXIT_FAILURE);
                        }

                        free(currentLineBuffer);
                        currentLineBuffer = NULL;
                        j = 0;
                        i++;

                        if (j > final_j && i > 1) {
                            final_j = j;
                        }
                    }
                } else if (test[l] == '\0') {
                    if (i > 0) {
                        currentLineBuffer[j] = '\0';
                        l = bytes_read;
                        data.sizeLines = i + 1;
                        data.sizeColumns = final_j + 1;
                        data.missingLines = 0;
                    }
                } else {
                    currentLineBuffer = realloc(currentLineBuffer, (j + 1) * sizeof(char));
                    if (currentLineBuffer == NULL) {
                        perror("Erreur r�allocation m�moire");
                        exit(EXIT_FAILURE);
                    }

                    currentLineBuffer[j] = test[l];
                    j++;
                }
            }
        }
        memset(buf, 0, sizeof(buf));
    } else if (bytes_read == 0) {
        printf("Le client a ferm� la connexion.\n");
        // Ajoutez ici le code de nettoyage ou de sortie appropri�
    } else {
        perror("Erreur de lecture depuis le client");
        exit(EXIT_FAILURE);
    }
}

// Lib�ration de la m�moire
free(currentLineBuffer);

// R�allocation finale pour data.data
data.data = realloc(data.data, data.sizeLines * sizeof(char *));
if (data.data == NULL) {
    perror("Erreur r�allocation m�moire");
    exit(EXIT_FAILURE);
}

    /*
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
                for (int l = 0; l < bytes_read; l++) {
                    // printf("i: %d, j: %d\n", i, j);
                    if (test[l] == '@') {
                        if (i > 0) {
                            currentLineBuffer[j] = '\0';
                            data.data[i] = strdup(currentLineBuffer);
                            free(currentLineBuffer);
                            currentLineBuffer = NULL;
                            j = 0;
                            i++;

                            if (j > final_j && i > 1) {
                                final_j = j;
                            }
                        }
                    } else if (test[l] == '\0') {
                        if (i > 0) {
                            currentLineBuffer[j] = '\0';
                            l = bytes_read;
                            data.sizeLines = i + 1;
                            data.sizeColumns = final_j + 1;
                            data.missingLines = 0;
                        }
                    } else {
                        currentLineBuffer = realloc(currentLineBuffer, (j + 1) * sizeof(char));
                        if (currentLineBuffer == NULL) {
                            perror("Erreur r�allocation m�moire");
                            exit(EXIT_FAILURE);
                        }

                        currentLineBuffer[j] = test[l];
                        j++;
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        }
    }

    data.data = realloc(data.data, data.sizeLines * sizeof(char *));
    if (data.data == NULL) {
        perror("Erreur r�allocation m�moire");
        exit(EXIT_FAILURE);
    }
*/
    freeData(&initial_data);

    fclose(resultat);
    fclose(fichier);

    printf("Taille du fichier initial : %d x %d\n", initial_data.sizeLines, initial_data.sizeColumns);
    printf("Taille du fichier final : %d x %d\n\n", data.sizeLines, data.sizeColumns);

    errorRate(initial_data, data);

    close(client);
    close(s);

    return 0;
}
/*#include <stdio.h>
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
    char **data;        // Ensemble de donn�es
    int sizeLines;      // Nombre de lignes de donn�es
    int sizeColumns;    // Nombre de colonnes de donn�es
    int missingLines;   // Nombre de lignes incompl�tes
} data_lines;

int set_l2cap_mtu(int s, uint16_t mtu) {
    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
}

data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");
    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier fichier.txt");
        exit(EXIT_FAILURE);
    }

    data_lines dataConverted;
    char **data;
    dataConverted.data = NULL;
    dataConverted.sizeColumns = dataConverted.sizeLines = dataConverted.missingLines = 0;
    int sizeColumns, sizeLines = 0;
    char currentChar;
    int c1 = 1;
    int c2 = 1;
    int c3 = 0;

    data = (char **)malloc(c1 * sizeof(char *));
    if (data == NULL) {
        perror("Erreur d'allocation m�moire");
        exit(EXIT_FAILURE);
    }

    data[c1 - 1] = (char *)malloc(c2 * sizeof(char));
    if (data[c1 - 1] == NULL) {
        perror("Erreur d'allocation m�moire");
        exit(EXIT_FAILURE);
    }

    while (!feof(fichier)) {
        currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {
            c1++;
            if (c1 == 2) {
                c3 = c2;
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 == c3 + 1 || c2 == c3 + 2 || c2 == c3 + 3 || c2 == c3 - 1 || c2 == c3 - 2 || c2 == c3 - 3) {
                    c3 = c2;
                    dataConverted.sizeColumns = c3;
                }
                if (c2 != c3) {
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(c2 - c3), c1 - 1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 1;
            data = (char **)realloc(data, c1 * sizeof(char *));
            if (data == NULL) {
                perror("Erreur r�allocation m�moire");
                exit(EXIT_FAILURE);
            }

            data[c1 - 1] = (char *)malloc(c2 * sizeof(char));
            if (data[c1 - 1] == NULL) {
                perror("Erreur d'allocation m�moire");
                exit(EXIT_FAILURE);
            }
        } else {
            c2++;
            data[c1 - 1] = (char *)realloc(data[c1 - 1], c2 * sizeof(char));
            if (data[c1 - 1] == NULL) {
                perror("Erreur r�allocation m�moire");
                exit(EXIT_FAILURE);
            }
            data[c1 - 1][c2 - 2] = currentChar;
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = c1;

    // Lib�ration de la m�moire allou�e pour chaque ligne du tableau
    for (int i = 0; i < dataConverted.sizeLines; i++) {
        free(data[i]);
    }

    // Lib�ration de la m�moire allou�e pour le tableau de pointeurs
    free(data);

    return dataConverted;
}

int errorRate(data_lines data1, data_lines data2) {
    float nb_errors = 0, nb_data = 0;
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

int main(int argc, char **argv) {
    char *bluetoothctl_command = "bluetoothctl";
    FILE *pipe = popen(bluetoothctl_command, "w");

    if (!pipe) {
        perror("Erreur lors de l'ouverture du tube.");
        return 1;
    }

    fprintf(pipe, "power on\n");
    fprintf(pipe, "discoverable yes\n");
    fprintf(pipe, "exit\n");

    fclose(pipe);

    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    data_lines data;
    data.data = (char **)malloc(sizeof(char *));
    data.data[0] = (char *)malloc(sizeof(char));
    data.data[0][0] = '\0';

    struct sched_param sched_p;
    sched_p.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) {
        perror("sched_setscheduler \n");
    }

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
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
    initial_data = DataConvert("/home/PERSEUS2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");

    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char));
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char));

    char *currentLineBuffer = NULL;
    size_t currentBufferSize = 0;

    FILE *fichier = fopen("test.txt", "w+");
    FILE *resultat = fopen("result.txt", "w+");

    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier test.txt");
    }
    if (resultat == NULL) {
        perror("Impossible d'ouvrir le fichier resultat.txt");
    }

    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    int check = 1;
    int i = 0, j = 0, final_j = 0;
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
                            if (i > 0) {
                                data.data[i] = currentLineBuffer;
                                currentLineBuffer = NULL;
                                j = 0;
                                i++;

                                if (j > final_j && i > 1) {
                                    final_j = j;
                                }
                            }
                        } else if (test[l] == '\0') {
                            if (i > 0) {
                                currentLineBuffer[j] = '\0';
                                l = bytes_read;
                                data.sizeLines = i + 1;
                                data.sizeColumns = final_j + 1;
                                data.missingLines = 0;
                            }
                        } else {
                            currentLineBuffer = (char *)realloc(currentLineBuffer, (j + 1) * sizeof(char));
                            if (currentLineBuffer == NULL) {
                                perror("Erreur r�allocation m�moire");
                                exit(EXIT_FAILURE);
                            }
                            currentLineBuffer[j] = test[l];
                            j++;
                        }
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        }
    }

    data.data = (char **)realloc(data.data, data.sizeLines * sizeof(char *));
    if (data.data == NULL) {
        perror("Erreur r�allocation m�moire");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < data.sizeLines; i++) {
        free(data.data[i]);
    }

    free(data.data);

    fclose(resultat);
    fclose(fichier);

    printf("Taille du fichier initial : %d x %d\n", initial_data.sizeLines, initial_data.sizeColumns);
    printf("Taille du fichier final : %d x %d\n\n", data.sizeLines, data.sizeColumns);

    errorRate(initial_data, data);

    close(client);
    close(s);

    return 0;
}
/*#include <stdio.h>
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
    char **data;        // Ensemble de donn�es
    int sizeLines;      // Nombre de lignes de donn�es
    int sizeColumns;    // Nombre de colonnes de donn�es
    int missingLines;   // Nombre de lignes incompl�tes
} data_lines;

int set_l2cap_mtu(int s, uint16_t mtu) { // Fonction qui change le MTU d'un socket
    printf("s", s);
    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
}

data_lines DataConvert(char *lien) { // Fonction de conversion des fichiers txt en char **

    // Ouverture du fichier
    FILE *fichier = fopen(lien, "r");
    // V�rification de l'ouverture du fichier
    if (fichier == NULL) {
        // On affiche un message d'erreur si on veut
        perror("Impossible d'ouvrir le fichier fichier.txt");
        exit(EXIT_FAILURE);
    }

    // Initialisations
    data_lines dataConverted;
    char **data;
    dataConverted.data = NULL;
    dataConverted.sizeColumns = dataConverted.sizeLines = dataConverted.missingLines = 0;
    int sizeColumns, sizeLines = 0;
    char currentChar;                   // Caract�re lu actuellement
    int c1 = 1;                         // Compteur de lignes
    int c2 = 1;                         // Compteur de colonnes
    int c3 = 0;                         // Compteur annexe

    // Allocation m�moire initiale
    data = (char **)malloc(c1 * sizeof(char *));

    if (data == NULL) {
        perror("Erreur d'allocation m�moire");
        exit(EXIT_FAILURE);
    }

    data[c1 - 1] = (char *)malloc(c2 * sizeof(char));
    if (data[c1 - 1] == NULL) {
        perror("Erreur d'allocation m�moire");
        exit(EXIT_FAILURE);
    }

    while (!feof(fichier)) {            // Tant qu'on est pas arriv�s � la fin du fichier

        // R�cup�ration du caract�re lu
        currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {            // Si on a un saut de ligne (car int "\n" = 10)

            c1++;

            if (c1 == 2) {
                c3 = c2;                    // Stockage du nombre de colonnes "normal" du fichier
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 == c3 + 1 || c2 == c3 + 2 || c2 == c3 + 3 || c2 == c3 - 1 || c2 == c3 - 2 || c2 == c3 - 3) {
                    // Pr�sence (ou non) des - dans les donn�es
                    c3 = c2;                 // Stockage du nouveau nombre "normal" de colonnes
                    dataConverted.sizeColumns = c3;
                }
                if (c2 != c3) {
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(c2 - c3), c1 - 1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 1;                         // Retour � la premi�re colonne

            // R�allocation m�moire pour la nouvelle ligne
            data = (char **)realloc(data, c1 * sizeof(char *));
            if (data == NULL) {
                perror("Erreur r�allocation m�moire");
                exit(EXIT_FAILURE);
            }

            data[c1 - 1] = (char *)malloc(c2 * sizeof(char));
            if (data == NULL) {
                perror("Erreur d'allocation m�moire");
                exit(EXIT_FAILURE);
            }

        } else {

            c2++;

            // R�allocation m�moire pour la nouvelle colonne
            data[c1 - 1] = (char *)realloc(data[c1 - 1], c2 * sizeof(char));
            if (data[c1 - 1] == NULL) {
                perror("Erreur r�allocation m�moire");
                exit(EXIT_FAILURE);
            }

            data[c1 - 1][c2 - 2] = currentChar;
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = c1;
    
    // Lib�ration de la m�moire allou�e pour chaque ligne du tableau
    for (int i = 0; i < dataConverted.sizeLines; i++) {
        free(data[i]);
    }

    // Lib�ration de la m�moire allou�e pour le tableau de pointeurs
    free(data);
    
    return dataConverted;
}

int errorRate(data_lines data1, data_lines data2) { // Fonction calculant le taux de perte et d'erreur dans la transmission

    // Initialisations
    float nb_errors = 0, nb_data = 0;
    double loss_rate, error_rate = 0.0;
    int minLines, maxColumns, deltaLines = 0;

    // Transformations en float (pour les divisions)
    float Lines1 = data1.sizeLines;
    float mLines1 = data1.missingLines;
    float Lines2 = data2.sizeLines;
    float mLines2 = data2.missingLines;

    // Calcul du taux de perte
    if (Lines1 >= Lines2) { // Le fichier 1 est plus long que le 2 (ou de m�me taille)
        loss_rate = (Lines1 - Lines2 + mLines1 + mLines2) / Lines1 * 100;
        minLines = Lines2;
        deltaLines = Lines1 - Lines2;
        maxColumns = data1.sizeColumns;
    } else { // Le fichier 2 est plus long que le 1
        loss_rate = (Lines2 - Lines1 + mLines1 + mLines2) / Lines2 * 100;
        minLines = Lines1;
        deltaLines = Lines2 - Lines1;
        maxColumns = data1.sizeColumns;
    }

    // Calcul du taux de perte de donn�es
    printf("Taux de perte de %.10lf pourcents\n", loss_rate);

    // Initialisations
    int i, j = 0;

    // Calcul du taux d'erreur
    for (i = 0; i < minLines; i++) { // On doit prendre la plus petite longueur de fichier pour �viter de parcourir un fichier fini
        for (j = 0; j < maxColumns; j++) {
            nb_data++;
            if (data1.data[i][j] != data2.data[i][j]) { // Si les char sont diff�rents
                nb_errors++;
            }
        }
    }

    // Ajout des lignes manquantes (car on a pris la plus petite longueur de fichier possible)
    nb_errors += deltaLines * maxColumns;

    // Calcul du taux d'erreur dans les donn�es
    error_rate = nb_errors / nb_data * 100;
    printf("Taux d'erreur de %.10lf pourcents\n", error_rate);

    return 0;
}
int main(int argc, char **argv) { // Fonction de r�ception des donn�es

    // Commande pour ex�cuter bluetoothctl
    char *bluetoothctl_command = "bluetoothctl";

    // Ouvrir un tube pour envoyer des commandes � bluetoothctl
    FILE *pipe = popen(bluetoothctl_command, "w");

    if (!pipe) {
        perror("Erreur lors de l'ouverture du tube.");
        return 1;
    }

    // Envoyer les commandes � bluetoothctl
    fprintf(pipe, "power on\n");
    fprintf(pipe, "discoverable yes\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);

    // Cr�ation du socket de r�ception et initialisations des donn�es
    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0}; // struct de socket
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    // Cr�ation du data_lines final
    data_lines data;
    data.data = (char **)malloc(sizeof(char *));
    data.data[0] = (char *)malloc(sizeof(char));
    data.data[0][0] = '\0';

    // D�finition de la priorit� du script en priorit� temps r�el
    struct sched_param sched_p; // Cr�ation d'une structure d'ordonnancement temps r�el pour le programme
    sched_p.sched_priority = 50; // Affectation d'une priorit� temps r�el entre 0 et 99
    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) { // Affectation d'un ordonancement Round-robin avec le param�tre de priorit� d�fini pr�c�demment si l'op�ration se passe sans erreur
        perror("sched_setscheduler \n"); // Sinon le programme se termine via la fonction perror()
    }

    // Allocation du socket
    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s, 1);

    if (s == -1) { // v�rification socket code de Bourdeaud'hui
        perror("socket");
        exit(1);
    }

    // Modification du MTU
    int mtu_value = 15620; // Valeur modifi�e par le fichier python
    set_l2cap_mtu(s, mtu_value);

    // Conversion du fichier initial
    data_lines initial_data;
    initial_data = DataConvert("/home/PERSEUS2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");

    // Initialisations de r�ception
    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char)); // Initialisation du buffer avec des z�ros
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char)); // Initialisation de test avec des z�ros

    // Initialisation de la variable tampon pour une ligne
    char *currentLineBuffer = NULL;
    size_t currentBufferSize = 0;

    // Ouverture du fichier de r�sultat et du fichier de r�ception des donn�es
    FILE *fichier = fopen("test.txt", "w+");
    FILE *resultat = fopen("result.txt", "w+");

    // V�rification ouverture des fichiers test et r�sultat
    if (fichier == NULL) {
        // On affiche un message d'erreur si on veut
        perror("Impossible d'ouvrir le fichier test.txt");
    }
    if (resultat == NULL) {
        // On affiche un message d'erreur si on veut
        perror("Impossible d'ouvrir le fichier resultat.txt");
    }

    // Acceptation de la connexion entre les Raspberry
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    // R�ception de donn�es
    int check = 1;
    int i = 0, j = 0, final_j = 0;
    long tempsboucle, temps_envoi = 0;
    struct timeval start, end; // Initialisation de variables de temps

    gettimeofday(&start, NULL); // Initialisation du temps de fin
    while (check) {
        bytes_read = recv(client, buf, mtu_value, 0); // R�ception des donn�es du client
        if (bytes_read > 0) {
            if (strcmp(buf, "stop") == 0) { // Quand on est arriv�s � la fin de l'envoi
                check = 0;
                gettimeofday(&end, NULL); // Initialisation du temps de fin
                temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
                printf("\nTemps de transmission : %ld micro secondes\n\n", temps_envoi);
                // Temps total de transmission
                fprintf(resultat, "%ld ms\n", temps_envoi);
            } else {
                strcpy(test, buf); // Copie du buffer vers la m�moire "test"
                if (fichier != NULL) {
                    fprintf(fichier, test); // On �crit dans le fichier la m�moire "test"
                    int l = 0;
                    for (l = 0; l < bytes_read; l++) { // Pour chaque caract�re lu
                        if (test[l] == '@') {          // Si on a un saut de ligne
                            if (i > 0) {
                                data.data[i] = currentLineBuffer;
                                currentLineBuffer = NULL;  // Remettre � NULL pour �viter la double lib�ration
                                j = 0;
                                i++;

                                if (j > final_j && i > 1) {
                                    final_j = j; // Afin de d�terminer le nombre de colonnes maximal
                                }
                            }
                        } else if (test[l] == '\0') { // Si on est � la fin des donn�es
                            if (i > 0) {
                                currentLineBuffer[j] = '\0';
                                l = bytes_read;
                                data.sizeLines = i + 1; // Initialisation des derniers param�tres de data_lines
                                data.sizeColumns = final_j + 1;
                                data.missingLines = 0; // Il faudrait rajouter une v�rification des lignes incompl�tes
                            }
                        } else {
                            // Ajouter le caract�re au tampon de la ligne actuelle
                            currentLineBuffer = (char *)realloc(currentLineBuffer, (j + 1) * sizeof(char));
                            if (currentLineBuffer == NULL) {
                                perror("Erreur r�allocation m�moire");
                                exit(EXIT_FAILURE);
                            }

                            currentLineBuffer[j] = test[l];
                            j++;
                        }
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        } 
    }

    // R�allouer la m�moire pour le tableau de pointeurs apr�s la boucle
    data.data = (char **)realloc(data.data, data.sizeLines * sizeof(char *));
    if (data.data == NULL) {
        perror("Erreur r�allocation m�moire");
        exit(EXIT_FAILURE);
    }

    // Lib�ration de la m�moire allou�e pour chaque ligne du tableau
    for (int i = 0; i < data.sizeLines; i++) {
        free(data.data[i]);
    }

    // Lib�ration de la m�moire allou�e pour le tableau de pointeurs
    free(data.data);
    
    
    //Fermeture des fichiers
	fclose(resultat);
	fclose(fichier);
	
	//Taille des data_lines
	printf("Taille du fichier initial : %d x %d\n", initial_data.sizeLines, initial_data.sizeColumns);
	printf("Taille du fichier final : %d x %d\n\n", data.sizeLines, data.sizeColumns);
	
	//Calcul du taux de perte et d'erreur
	errorRate(initial_data,data);

	//Fermeture des sockets
	close (client) ;
	close (s) ;


        return 0;
}
    
/*
int main(int argc, char **argv) { // Fonction de r�ception des donn�es

    // Commande pour ex�cuter bluetoothctl
    char *bluetoothctl_command = "bluetoothctl";

    // Ouvrir un tube pour envoyer des commandes � bluetoothctl
    FILE *pipe = popen(bluetoothctl_command, "w");

    if (!pipe) {
        perror("Erreur lors de l'ouverture du tube.");
        return 1;
    }

    // Envoyer les commandes � bluetoothctl
    fprintf(pipe, "power on\n");
    fprintf(pipe, "discoverable yes\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);

    // Cr�ation du socket de r�ception et initialisations des donn�es
    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0}; // struct de socket
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    // Cr�ation du data_lines final
    data_lines data;
    data.data = (char **)malloc(sizeof(char *));
    data.data[0] = (char *)malloc(sizeof(char));
    data.data[0][0] = '\0';

    // D�finition de la priorit� du script en priorit� temps r�el
    struct sched_param sched_p; // Cr�ation d'une structure d'ordonnancement temps r�el pour le programme
    sched_p.sched_priority = 50; // Affectation d'une priorit� temps r�el entre 0 et 99
    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) { // Affectation d'un ordonancement Round-robin avec le param�tre de priorit� d�fini pr�c�demment si l'op�ration se passe sans erreur
        perror("sched_setscheduler \n"); // Sinon le programme se termine via la fonction perror()
    }

    // Allocation du socket
    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s, 1);

    if (s == -1) { // v�rification socket code de Bourdeaud'hui
        perror("socket");
        exit(1);
    }

    // Modification du MTU
    int mtu_value = 15620; // Valeur modifi�e par le fichier python
    set_l2cap_mtu(s, mtu_value);

    // Conversion du fichier initial
    data_lines initial_data;
    initial_data = DataConvert("/home/PERSEUS2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");

    // Initialisations de r�ception
    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char)); // Initialisation du buffer avec des z�ros
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char)); // Initialisation de test avec des z�ros

    // Ouverture du fichier de r�sultat et du fichier de r�ception des donn�es
    FILE *fichier = fopen("test.txt", "w+");
    FILE *resultat = fopen("result.txt", "w+");

    // V�rification ouverture des fichiers test et r�sultat
    if (fichier == NULL) {
        // On affiche un message d'erreur si on veut
        perror("Impossible d'ouvrir le fichier test.txt");
    }
    if (resultat == NULL) {
        // On affiche un message d'erreur si on veut
        perror("Impossible d'ouvrir le fichier resultat.txt");
    }

    // Acceptation de la connexion entre les Raspberry
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    // R�ception de donn�es
    int check = 1;
    int i, j, final_j = 0;
    long tempsboucle, temps_envoi = 0;
    struct timeval start, end; // Initialisation de variables de temps

    gettimeofday(&start, NULL); // Initialisation du temps de fin
    
   while (check) {
    bytes_read = recv(client, buf, mtu_value, 0); // R�ception des donn�es du client
    if (bytes_read > 0) {
        if (strcmp(buf, "stop") == 0) { // Quand on est arriv�s � la fin de l'envoi
            check = 0;
            gettimeofday(&end, NULL); // Initialisation du temps de fin
            temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
            printf("\nTemps de transmission : %ld micro secondes\n\n", temps_envoi);
            // Temps total de transmission
            fprintf(resultat, "%ld ms\n", temps_envoi);
        } else {
            strcpy(test, buf); // Copie du buffer vers la m�moire "test"
            if (fichier != NULL) {
                fprintf(fichier, test); // On �crit dans le fichier la m�moire "test"
                int l = 0;
                for (l = 0; l < bytes_read; l++) { // Pour chaque caract�re lu
                    if (test[l] == '@') {          // Si on a un saut de ligne
                        data.data[i][j] = '\n';
                        i++;
                        if (j > final_j && i > 1) {
                            final_j = j; // Afin de d�terminer le nombre de colonnes maximal
                        }
                        j = 0;
                        data.data = (char **)realloc(data.data, (i + 1) * sizeof(char *));
                        if (data.data == NULL) {
                            perror("Erreur r�allocation m�moire");
                            exit(EXIT_FAILURE);
                        }
                        // R�allocation d'une nouvelle ligne
                        data.data[i] = (char *)malloc(sizeof(char));
                        if (data.data[i] == NULL) {
                            perror("Erreur d'allocation m�moire");
                            exit(EXIT_FAILURE);
                        }
                    } else if (test[l] == '\0') { // Si on est � la fin des donn�es
                        data.data[i][j] = '\0';
                        l = bytes_read;
                        data.sizeLines = i + 1; // Initialisation des derniers param�tres de data_lines
                        data.sizeColumns = final_j + 1;
                        data.missingLines = 0; // Il faudrait rajouter une v�rification des lignes incompl�tes
                    } else {
                        data.data[i][j] = test[l]; // Ajout au data
                        j++;
                        data.data[i] = (char *)realloc(data.data[i], (j + 1) * sizeof(char)); // R�allocation m�moire
                        if (data.data[i] == NULL) {
                            perror("Erreur r�allocation m�moire");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
        memset(buf, 0, sizeof(buf));
}
}
}
*/


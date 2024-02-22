
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

#define MAX_LINES 144171
#define MAX_COLUMNS 4

// Structure pour stocker les données lues depuis le fichier
typedef struct {
    char data[MAX_LINES][MAX_COLUMNS];
    int sizeLines;
    int sizeColumns;
    int missingLines;
} data_lines;

// Déclaration des variables pour le calcul du temps d'envoi
struct timeval start, end;
long int temps_envoi;

// Fonction pour configurer le MTU d'un socket L2CAP
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

// Fonction pour convertir les données à partir d'un fichier
data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");

    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier");
        exit(EXIT_FAILURE);
    }

    data_lines dataConverted;
    dataConverted.sizeLines = 0;
    dataConverted.sizeColumns = MAX_COLUMNS;
    dataConverted.missingLines = 0;

    int c1 = 0;  // Compteur de lignes
    int c2 = 0;  // Compteur de colonnes

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {  // Si on a un saut de ligne (car int "\n" = 10)
            c1++;
            c2 = 0;

            if (c1 > dataConverted.sizeLines) {
                dataConverted.sizeLines = c1;
            }
        } else {
            if (c1 == 0) {
                dataConverted.sizeLines = 1;
            }

            dataConverted.data[c1][c2] = currentChar;
            c2++;

            if (c2 >= dataConverted.sizeColumns) {
                // Ignorer les caractères supplémentaires dans la même ligne
                while ((currentInt = fgetc(fichier)) != 10 && currentInt != EOF)
                    ;
            }
        }
    }

    fclose(fichier);
    return dataConverted;
}

// Fonction pour calculer le taux d'erreur entre deux ensembles de données
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

int main(int argc, char **argv) {
    // Commande pour exécuter bluetoothctl
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
    printf("bluetooth ok\n");

    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    data_lines data;
    data.sizeLines = MAX_LINES;
    data.sizeColumns = MAX_COLUMNS;
    data.missingLines = 0;

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
    printf("fichier ouvert avec lien ok\n");

    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char));

    FILE *fichier = fopen("test.txt", "w+");
    FILE *resultat = fopen("result.txt", "w+");

    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier test.txt");
        exit(EXIT_FAILURE);
    }

    if (resultat == NULL) {
        perror("Impossible d'ouvrir le fichier resultat.txt");
        exit(EXIT_FAILURE);
    }

    printf("ouverture fichier ok\n");

    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    printf("connexion ok\n");

    int check = 1;
    while (check) {
        gettimeofday(&start, NULL);
        //printf("initialisation reception ok\n");

        bytes_read = recv(client, buf, sizeof(buf), 0);

        gettimeofday(&end, NULL);
        temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));

        if (bytes_read > 0) {
            fprintf(fichier, "%s", buf);
            fprintf(resultat, "%ld\n", temps_envoi);
        } else {
            fprintf(stderr, "Erreur de lecture depuis le client\n");
            exit(EXIT_FAILURE);
        }

        usleep(50000);  // temps d'attente pour laisser le temps de lire le buffer
    }

    fclose(fichier);
    fclose(resultat);

    // Compare les fichiers et calcule les taux d'erreur
    data_lines received_data = DataConvert("test.txt");
    errorRate(initial_data, received_data);

    close(client);
    close(s);
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

#define MAX_LINES 144171
#define MAX_COLUMNS 4

typedef struct {
    char data[MAX_LINES][MAX_COLUMNS];
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
    dataConverted.sizeLines = 0;
    dataConverted.sizeColumns = MAX_COLUMNS;
    dataConverted.missingLines = 0;

    int c1 = 0;  // Compteur de lignes
    int c2 = 0;  // Compteur de colonnes

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {  // Si on a un saut de ligne (car int "\n" = 10)
            c1++;
            c2 = 0;

            if (c1 > dataConverted.sizeLines) {
                dataConverted.sizeLines = c1;
            }
        } else {
            if (c1 == 0) {
                dataConverted.sizeLines = 1;
            }

            dataConverted.data[c1][c2] = currentChar;
            c2++;

            if (c2 >= dataConverted.sizeColumns) {
                // Ignorer les caractères supplémentaires dans la même ligne
                while ((currentInt = fgetc(fichier)) != 10 && currentInt != EOF)
                    ;
            }
        }
    }

    fclose(fichier);
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
    printf("bluetooth ok\n");

    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    data_lines data;
    data.sizeLines = MAX_LINES;
    data.sizeColumns = MAX_COLUMNS;
    data.missingLines = 0;

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
    printf("fichier ouvert avec lien ok\n");

    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char));
    char test[mtu_value];
    memset(test, 0, mtu_value * sizeof(char));

    FILE *fichier = fopen("test.txt", "w+");
    FILE *resultat = fopen("result.txt", "w+");

    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier test.txt");
        exit(EXIT_FAILURE);
    }

    if (resultat == NULL) {
        perror("Impossible d'ouvrir le fichier resultat.txt");
        exit(EXIT_FAILURE);
    }

    printf("ouverture fichier ok\n");

    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    printf("connexion ok\n");

    int check = 1;
    int i = 0, j = 0, final_j = 0;
    long tempsboucle, temps_envoi = 0;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    printf("initialisation reception ok\n");
 // Acceptation de la connexion entre les Raspberry
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    if (client == -1) {
        perror("Erreur lors de l'acceptation de la connexion");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Connexion acceptée depuis %s\n", buf);
    memset(buf, 0, sizeof(buf));

    if (client < 0) {
        perror("Erreur lors de la connexion avec le client");
        exit(EXIT_FAILURE);
    }

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
                        //printf("i: %d, j: %d\n", i, j);
                        if (test[l] == '@') {
                            data.data[i][j] = '\n';
                            i++;
                            if (j > final_j && i > 1) {
                                final_j = j;
                            }
                            j = 0;
                        } else if (test[l] == '\0') {
                            data.data[i][j] = '\0';
                            l = bytes_read;
                            data.sizeLines = i;
                            data.sizeColumns = final_j;
                            data.missingLines = 0;
                        } else {
                            data.data[i][j] = test[l];
                            j++;
                        }
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        } else {
            fprintf(stderr, "Erreur de lecture depuis le client\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("reception donné ok\n");

    fclose(resultat);
    fclose(fichier);

    printf("Taille du fichier initial : %d x %d\n", initial_data.sizeLines, initial_data.sizeColumns);
    printf("Taille du fichier final : %d x %d\n\n", data.sizeLines, data.sizeColumns);

    errorRate(initial_data, data);

    close(client);
    close(s);

    return 0;
}
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Ajout de l'en-tête pour la fonction strerror
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

typedef struct {
    char **data;        // Ensemble de données
    int sizeLines;      // Nombre de lignes de données
    int sizeColumns;    // Nombre de colonnes de données
    int missingLines;   // Nombre de lignes incomplètes
} data_lines;

data_lines DataConvert(char *lien) {
    // Ouverture du fichier
    FILE *fichier = fopen(lien, "r");
    
    // Initialisations
    data_lines dataConverted = {0}; // Initialisation des valeurs à zéro
    char **data = NULL;
    int c1 = 0; // Compteur de lignes
    int c2 = 0; // Compteur de colonnes
    int c3 = 0; // Compteur annexe

    if (fichier == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        exit(1);
    }

    // Allocation mémoire initiale
    data = (char **)malloc(sizeof(char *));
    data[0] = NULL;

    int currentChar;
    int nb_data = 0;

    while ((currentChar = fgetc(fichier)) != EOF) {
        if (currentChar == '\n') {
            c1++;
            if (c1 == 1) {
                c3 = c2;
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 != c3) {
                    printf("Il manque %d caractères dans la ligne %d du fichier %s\n", abs(c2 - c3), c1 - 1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 0;
           

            // Réallocation mémoire pour la nouvelle ligne
            data = (char **)realloc(data, (c1 + 1) * sizeof(char *));
            data[c1] = NULL;
           
        } else {
            c2++;
            data[c1] = (char *)realloc(data[c1], (c2 + 1) * sizeof(char));
            data[c1][c2 - 1] = (char)currentChar;
            
        }
        nb_data++;
        printf("test1");
        
    }
    

    dataConverted.data = data;
    dataConverted.sizeLines = c1;

    fclose(fichier);
    printf("test2");
   

    return dataConverted;
}

int set_l2cap_mtu(int s, uint16_t mtu) {
    struct l2cap_options opts;
    printf("test3");
    socklen_t optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    printf("test4");
    return status;
}

int envoie(data_lines data) {
    // Création du socket dans le but de connecter entre elles les 2 Raspberry
    struct sockaddr_l2 addr = {0};
    printf("test5");
    int s, status;
    char dest[18] = "dc:a6:32:78:6d:91";
    printf("test6");

    // Allocation du socket
    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (s < 0) {
        fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
        perror("Erreur lors de la création du socket\n");
        exit(1);
    }

    // Modification du MTU
    int mtu_value = 15620;
    set_l2cap_mtu(s, mtu_value);

    // Connexion entre les 2 Raspberry
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x1001);
    str2ba(dest, &addr.l2_bdaddr);
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    // Initialisations d'envoi
    struct timeval start, end;
    char paquet[mtu_value];
    memset(paquet, 0, mtu_value * sizeof(char));

    // Envoi de données
    if (status == 0) {
        printf("Connexion réussie\n");
        gettimeofday(&start, NULL);
        int nb_data = 0;

        for (int i = 0; i < data.sizeLines; i++) {
            for (int j = 0; j < data.sizeColumns; j++) {
                if ((nb_data + data.sizeColumns) >= mtu_value) {
                    send(s, paquet, mtu_value, 0);
                    memset(paquet, 0, mtu_value * sizeof(char));
                    nb_data = 0;
                }
                char c = data.data[i][j];
                if (c == 101 || c == 43 || c == 46 || c == 32 || c == 45 || (c >= 48 && c <= 57)) {
                    paquet[nb_data] = c;
                    nb_data++;
                }
            }
            paquet[nb_data] = '@';
            nb_data++;
        }

        send(s, "stop", 4, 0);
        gettimeofday(&end, NULL);
        printf("Temps de transmission : %ld microsecondes\n", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
    } else {
        fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
        perror("Connexion échouée\n");
    }

    close(s);

    return 0;
}

int main(int argc, char **argv) {
    data_lines data;
    data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
    envoie(data);
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_COLUMNS 4
#define MAX_LINES 144171
#define MTU_VALUE 672

typedef struct {
    char data[MAX_LINES][MAX_COLUMNS];
    int sizeLines;
    int sizeColumns;
    int missingLines;
} data_lines;

data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");
    data_lines dataConverted;
    dataConverted.sizeColumns = 0;
    dataConverted.sizeLines = 0;
    dataConverted.missingLines = 0;

    char data[MAX_LINES][MAX_COLUMNS];  // Utilisation d'un tableau statique

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {
            if (dataConverted.sizeLines == 0) {
                dataConverted.sizeColumns = dataConverted.sizeColumns;
            } else {
                if (dataConverted.sizeColumns != MAX_COLUMNS) {
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(dataConverted.sizeColumns - MAX_COLUMNS), dataConverted.sizeLines - 1, lien);
                    dataConverted.missingLines++;
                }
            }

            dataConverted.sizeColumns = 0;
            dataConverted.sizeLines++;
        } else {
            data[dataConverted.sizeLines][dataConverted.sizeColumns] = currentChar;
            dataConverted.sizeColumns++;
        }
    }

    // Copie des données dans la structure de données finale
    for (int i = 0; i < dataConverted.sizeLines; i++) {
        for (int j = 0; j < dataConverted.sizeColumns; j++) {
            dataConverted.data[i][j] = data[i][j];
        }
    }

    fclose(fichier);
    return dataConverted;
}

int set_l2cap_mtu(int s, uint16_t mtu) {
    struct l2cap_options opts;
    socklen_t optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);

    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }

    return status;
}

int envoie(data_lines data) {
    struct sockaddr_l2 addr = {0};
    int s, status;
    char dest[18] = "DC:A6:32:78:6D:92";

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (s == -1) {
        perror("socket");
        exit(1);
    }

    set_l2cap_mtu(s, MTU_VALUE);

    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x1001);
    str2ba(dest, &addr.l2_bdaddr);
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    if (status == -1) {
        perror("connect");
        exit(2);
    }

    int i, j = 0;
    struct timeval start, end;
    char paquet[MTU_VALUE];
    memset(paquet, 0, MTU_VALUE * sizeof(char));

    if (0 == status) {
        printf("Connexion réussie\n");
        gettimeofday(&start, NULL);

        int nb_data = 0;
        for (i = 0; i < data.sizeLines; i++) {
            if ((nb_data + data.sizeColumns) >= MTU_VALUE) {
                send(s, paquet, sizeof(paquet), 0);
                memset(paquet, 0, MTU_VALUE * sizeof(char));
                nb_data = 0;
            }

            int j = 0;
            int c = data.data[i][j];
            while (c == 101 || c == 43 || c == 46 || c == 32 || c == 45 || (c >= 48 && c <= 57)) {
                paquet[nb_data] = data.data[i][j];
                nb_data++;
                j++;
                c = data.data[i][j];
            }

            char arobase = 64;
            paquet[nb_data] = arobase;
            nb_data++;
        }

        send(s, "stop", 4, 0);
        gettimeofday(&end, NULL);
        printf("Temps de transmission : %ld micro seconds\n",
               ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
    }

    if (status < 0) {
        fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
        perror("Connexion echouée\n");
    }

    close(s);
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    fclose(pipe);

    printf("%d", 1);
    data_lines data;
    data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
    envoie(data);
}

/*

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_COLUMNS 4
#define MAX_LINES 144171
#define MTU_VALUE 672

typedef struct {
    char data[MAX_LINES][MAX_COLUMNS];
    int sizeLines;
    int sizeColumns;
    int missingLines;
} data_lines;

data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");
    data_lines dataConverted;
    dataConverted.sizeColumns = 0;
    dataConverted.sizeLines = 0;
    dataConverted.missingLines = 0;

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {
            if (dataConverted.sizeLines == 0) {
                dataConverted.sizeColumns = dataConverted.sizeColumns;
            } else {
                if (dataConverted.sizeColumns != MAX_COLUMNS) {
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(dataConverted.sizeColumns - MAX_COLUMNS), dataConverted.sizeLines - 1, lien);
                    dataConverted.missingLines++;
                }
            }

            dataConverted.sizeColumns = 0;
            dataConverted.sizeLines++;
        } else {
            dataConverted.data[dataConverted.sizeLines][dataConverted.sizeColumns] = currentChar;
            dataConverted.sizeColumns++;
        }
    }

    fclose(fichier);
    return dataConverted;
}

int set_l2cap_mtu(int s, uint16_t mtu) {
    struct l2cap_options opts;
    socklen_t optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);

    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }

    return status;
}

int envoie(data_lines data) {
    struct sockaddr_l2 addr = {0};
    int s, status;
    char dest[18] = "DC:A6:32:78:6D:92";

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (s == -1) {
        perror("socket");
        exit(1);
    }

    set_l2cap_mtu(s, MTU_VALUE);

    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x1001);
    str2ba(dest, &addr.l2_bdaddr);
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    if (status == -1) {
        perror("connect");
        exit(2);
    }

    int i, j = 0;
    struct timeval start, end;
    char paquet[MTU_VALUE];
    memset(paquet, 0, MTU_VALUE * sizeof(char));

    if (0 == status) {
        printf("Connexion réussie\n");
        gettimeofday(&start, NULL);

        int nb_data = 0;
        for (i = 0; i < data.sizeLines; i++) {
            if ((nb_data + data.sizeColumns) >= MTU_VALUE) {
                send(s, paquet, sizeof(paquet), 0);
                memset(paquet, 0, MTU_VALUE * sizeof(char));
                nb_data = 0;
            }

            int j = 0;
            int c = data.data[i][j];
            while (c == 101 || c == 43 || c == 46 || c == 32 || c == 45 || (c >= 48 && c <= 57)) {
                paquet[nb_data] = data.data[i][j];
                nb_data++;
                j++;
                c = data.data[i][j];
            }

            char arobase = 64;
            paquet[nb_data] = arobase;
            nb_data++;
        }

        send(s, "stop", 4, 0);
        gettimeofday(&end, NULL);
        printf("Temps de transmission : %ld micro seconds\n",
               ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
    }

    if (status < 0) {
        fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
        perror("Connexion echouée\n");
    }

    close(s);
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    fclose(pipe);

    printf("%d", 1);
    data_lines data;
    data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
    envoie(data);
}
/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_COLUMNS 4  // Remplacez par la valeur correcte
#define MAX_LINES 144171   // Remplacez par la valeur correcte
#define MTU_VALUE 672  // Ajout de la valeur MTU correcte

typedef struct {
    char data[MAX_LINES][MAX_COLUMNS];  // Ensemble de données
    int sizeLines;                      // Nombre de lignes de données
    int sizeColumns;                    // Nombre de colonnes de données
    int missingLines;                   // Nombre de lignes incomplètes
} data_lines;

data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");
    data_lines dataConverted;
    dataConverted.sizeColumns = 0;
    dataConverted.sizeLines = 0;
    dataConverted.missingLines = 0;

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {  // Si on a un saut de ligne (car int "\n" = 10)
            if (dataConverted.sizeLines == 0) {
                dataConverted.sizeColumns = dataConverted.sizeColumns;
            } else {
                if (dataConverted.sizeColumns != MAX_COLUMNS) {  // Correction ici
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(dataConverted.sizeColumns - MAX_COLUMNS), dataConverted.sizeLines - 1, lien);
                    dataConverted.missingLines++;
                }
            }

            dataConverted.sizeColumns = 0;
            dataConverted.sizeLines++;
        } else {
            dataConverted.data[dataConverted.sizeLines][dataConverted.sizeColumns] = currentChar;
            dataConverted.sizeColumns++;
        }
    }

    fclose(fichier);
    return dataConverted;
}

int set_l2cap_mtu(int s, uint16_t mtu) {
    struct l2cap_options opts;
    socklen_t optlen = sizeof(opts);  // Correction ici
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);

    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }

    return status;
}

int envoie(data_lines data) {
    struct sockaddr_l2 addr = {0};
    int s, status;
    char dest[18] = "DC:A6:32:78:6D:92";  // Correction ici

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (s == -1) {
        perror("socket");
        exit(1);
    }

    set_l2cap_mtu(s, MTU_VALUE);  // Correction ici

    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x1001);
    str2ba(dest, &addr.l2_bdaddr);
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    if (status == -1) {
        perror("connect");
        exit(2);
    }

    int i, j = 0;
    struct timeval start, end;
    char paquet[MTU_VALUE];  // Correction ici
    memset(paquet, 0, MTU_VALUE * sizeof(char));

    if (0 == status) {
        printf("Connexion réussie\n");
        gettimeofday(&start, NULL);

        int nb_data = 0;
        for (i = 0; i < data.sizeLines; i++) {
            if ((nb_data + data.sizeColumns) >= MTU_VALUE) {  // Correction ici
                send(s, paquet, sizeof(paquet), 0);
                memset(paquet, 0, MTU_VALUE * sizeof(char));
                nb_data = 0;
            }

            int j = 0;
            int c = data.data[i][j];
            while (c == 101 || c == 43 || c == 46 || c == 32 || c == 45 || (c >= 48 && c <= 57)) {
                paquet[nb_data] = data.data[i][j];
                nb_data++;
                j++;
                c = data.data[i][j];
            }

            char arobase = 64;
            paquet[nb_data] = arobase;
            nb_data++;
        }

        send(s, "stop", 4, 0);
        gettimeofday(&end, NULL);
        printf("Temps de transmission : %ld micro seconds\n",
               ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
    }

    if (status < 0) {
        fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
        perror("Connexion echouée\n");
    }

    close(s);
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    fclose(pipe);

    printf("%d", 1);
    data_lines data;
    data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
    envoie(data);
}



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_COLUMNS 4  // Remplacez par la valeur correcte
#define MAX_LINES 144171   // Remplacez par la valeur correcte


typedef struct {
    char data[MAX_LINES][MAX_COLUMNS];  // Ensemble de données
    int sizeLines;                      // Nombre de lignes de données
    int sizeColumns;                    // Nombre de colonnes de données
    int missingLines;                   // Nombre de lignes incomplètes
} data_lines;

data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");
    data_lines dataConverted;
    dataConverted.sizeColumns = 0;
    dataConverted.sizeLines = 0;
    dataConverted.missingLines = 0;

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {  // Si on a un saut de ligne (car int "\n" = 10)
            if (dataConverted.sizeLines == 0) {
                dataConverted.sizeColumns = dataConverted.sizeColumns;
            } else {
                if (dataConverted.sizeColumns != dataConverted.sizeColumns) {
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(dataConverted.sizeColumns - dataConverted.sizeColumns), dataConverted.sizeLines - 1, lien);
                    dataConverted.missingLines++;
                }
            }

            dataConverted.sizeColumns = 0;
            dataConverted.sizeLines++;
        } else {
            dataConverted.data[dataConverted.sizeLines][dataConverted.sizeColumns] = currentChar;
            dataConverted.sizeColumns++;
        }
    }

    fclose(fichier);
    return dataConverted;
}

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

int envoie(data_lines data) {
    struct sockaddr_l2 addr = {0};
    int s, status;
    char dest[22] = "dc:a6:32:78:6d:92";

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (s == -1) {
        perror("socket");
        exit(1);
    }

    int mtu_value = 15620;
    set_l2cap_mtu(s, mtu_value);

    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x1001);
    str2ba(dest, &addr.l2_bdaddr);
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    if (status == -1) {
        perror("connect");
        exit(2);
    }

    int i, j = 0;
    struct timeval start, end;
    char paquet[mtu_value];
    memset(paquet, 0, mtu_value * sizeof(char));

    if (0 == status) {
        printf("Connexion réussie\n");
        gettimeofday(&start, NULL);

        int nb_data = 0;
        for (i = 0; i < data.sizeLines; i++) {
            if ((nb_data + data.sizeColumns) >= mtu_value) {
                send(s, paquet, sizeof(paquet), 0);
                memset(paquet, 0, mtu_value * sizeof(char));
                nb_data = 0;
            }

            int j = 0;
            int c = data.data[i][j];
            while (c == 101 || c == 43 || c == 46 || c == 32 || c == 45 || (c >= 48 && c <= 57)) {
                paquet[nb_data] = data.data[i][j];
                nb_data++;
                j++;
                c = data.data[i][j];
            }

            char arobase = 64;
            paquet[nb_data] = arobase;
            nb_data++;
        }

        send(s, "stop", 4, 0);
        gettimeofday(&end, NULL);
        printf("Temps de transmission : %ld micro seconds\n",
               ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
    }

    if (status < 0) {
        fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
        perror("Connexion echouée\n");
    }

    close(s);
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    fclose(pipe);

    printf("%d", 1);
    data_lines data;
    data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
    envoie(data);
}

*/
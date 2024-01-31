// autre test
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 500

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/test2.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    clock_t start_time = clock();  // Nouveau point de départ pour la mesure du temps

    while (1) {
        size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
        if (bytesRead <= 0) {
            // Fin du fichier
            break;
        }

        size_t totalSent = 0;
        while (totalSent < bytesRead) {
            ssize_t sentBytes = send(sock, buffer + totalSent, bytesRead - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }
    }

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps pris pour envoyer le fichier complet : %f secondes\n", elapsed_time);

    fclose(file);
    close(sock);

    return 0;
}

/*
// donne le temps final et envoie tout le fichier--> trop grand
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 1024  // Taille du tampon ajustée à la valeur optimale du MTU

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/test2.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    clock_t start_time = clock();  // Nouveau point de départ pour la mesure du temps

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        size_t totalSent = 0;
        while (totalSent < bytesRead) {
            ssize_t sentBytes = send(sock, buffer + totalSent, bytesRead - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }
    }

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps pris pour envoyer le fichier complet : %f secondes\n", elapsed_time);

    fclose(file);
    close(sock);

    return 0;
}

/*
// cette fois il devrait envoyer que la premère ligne--> fonctionne
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/test2.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        clock_t start_time = clock();

        size_t totalSent = 0;
        size_t lineLength = strlen(buffer);

        while (totalSent < lineLength) {
            ssize_t sentBytes = send(sock, buffer + totalSent, lineLength - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }

        clock_t end_time = clock();
        double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Temps pris pour envoyer la première ligne : %f secondes\n", elapsed_time);
    }

    fclose(file);
    close(sock);

    return 0;
}



/*
//autre tentative--> il envoie tout
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/test2.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    char *bytesRead;

    while ((bytesRead = fgets(buffer, sizeof(buffer), file)) != NULL) {
        clock_t start_time = clock();

        size_t totalSent = 0;
        size_t lineLength = strlen(buffer);

        while (totalSent < lineLength) {
            ssize_t sentBytes = send(sock, buffer + totalSent, lineLength - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }

        clock_t end_time = clock();
        double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Temps pris pour envoyer une ligne : %f secondes\n", elapsed_time);
    }

    fclose(file);
    close(sock);

    return 0;
}


/*
//envoie première ligne --> erreur
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/test2.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    if ((bytesRead = fgets(buffer, sizeof(buffer), file)) != NULL) {
        size_t totalSent = 0;
        size_t lineLength = strlen(buffer);

        while (totalSent < lineLength) {
            ssize_t sentBytes = send(sock, buffer + totalSent, lineLength - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }

        printf("Première ligne envoyée avec succès.\n");
    } else {
        printf("Le fichier est vide.\n");
    }

    fclose(file);
    close(sock);

    return 0;
}

/*
//calcul temps d'un envoie

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <destination_address> <lines_per_send>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt";  // Chemin du fichier à envoyer
    int lines_per_send = atoi(argv[2]);

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    int lines_sent = 0;

    clock_t start_time;
    double total_elapsed_time = 0.0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        size_t totalSent = 0;
        size_t lineLength = strlen(buffer);

        while (totalSent < lineLength) {
            ssize_t sentBytes = send(sock, buffer + totalSent, lineLength - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }

        lines_sent++;

        if (lines_sent >= lines_per_send) {
            clock_t end_time = clock();
            double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
            total_elapsed_time += elapsed_time;

            printf("Temps pris pour envoyer %d lignes : %f secondes\n", lines_per_send, total_elapsed_time);

            printf("Pausing for a moment before sending the next batch.\n");
            usleep(500000);  // Pause de 0.5 seconde entre les envois
            lines_sent = 0;  // Réinitialiser le compteur de lignes envoyées
            start_time = clock();  // Nouveau point de départ pour le prochain lot
        }
    }

    fclose(file);
    close(sock);

    return 0;
}


/*
//choisir le nombre de ligne
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <destination_address> <lines_per_send>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt";  // Chemin du fichier à envoyer
    int lines_per_send = atoi(argv[2]);

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    int lines_sent = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        clock_t start_time = clock();

        size_t totalSent = 0;
        size_t lineLength = strlen(buffer);

        while (totalSent < lineLength) {
            ssize_t sentBytes = send(sock, buffer + totalSent, lineLength - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }

        lines_sent++;

        clock_t end_time = clock();
        double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Temps pris pour envoyer une ligne : %f secondes\n", elapsed_time);

        if (lines_sent >= lines_per_send) {
            printf("Pausing for a moment before sending the next batch.\n");
            usleep(500000);  // Pause de 0.5 seconde entre les envois
            lines_sent = 0;  // Réinitialiser le compteur de lignes envoyées
        }
    }

    fclose(file);
    close(sock);

    return 0;
}


//taille mtu des g2 trop long
/*#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 15620  // Taille du tampon ajustée à la valeur optimale du MTU

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        clock_t start_time = clock();

        size_t totalSent = 0;
        while (totalSent < bytesRead) {
            ssize_t sentBytes = send(sock, buffer + totalSent, bytesRead - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }

        clock_t end_time = clock();
        double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Temps pris pour envoyer une quantité optimale : %f secondes\n", elapsed_time);
    }

    fclose(file);
    close(sock);

    return 0;
}
*/
/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        clock_t start_time = clock();

        size_t totalSent = 0;
        size_t lineLength = strlen(buffer);

        while (totalSent < lineLength) {
            ssize_t sentBytes = send(sock, buffer + totalSent, lineLength - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }

        clock_t end_time = clock();
        double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Temps pris pour envoyer une ligne : %f secondes\n", elapsed_time);
    }

    fclose(file);
    close(sock);

    return 0;
}

/*#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        size_t totalSent = 0;
        size_t lineLength = strlen(buffer);

        while (totalSent < lineLength) {
            ssize_t sentBytes = send(sock, buffer + totalSent, lineLength - totalSent, 0);
            if (sentBytes == -1) {
                perror("send");
                fclose(file);
                close(sock);
                exit(EXIT_FAILURE);
            }
            totalSent += sentBytes;
            usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
        }
    }

    fclose(file);
    close(sock);

    return 0;
}
/*#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#define BUFFER_SIZE 124

int main(int argc, char *argv[]) {

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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t sentBytes = send(sock, buffer, bytesRead, 0);
        if (sentBytes == -1) {
            perror("send");
            break;
        } else if (sentBytes != bytesRead) {
            fprintf(stderr, "Incomplete send\n");
            break;
        }
        usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
    }

    fclose(file);
    close(sock);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main(int argc, char *argv[]) {
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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t sentBytes = send(sock, buffer, bytesRead, 0);
        if (sentBytes == -1) {
            perror("send");
            break;
        } else if (sentBytes != bytesRead) {
            fprintf(stderr, "Incomplete send\n");
            break;
        }
        usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
    }

    fclose(file);
    close(sock);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main(int argc, char *argv[]) {


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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <destination_address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = argv[1];
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/test1.txt";  // Chemin du fichier à envoyer

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(sock, buffer, bytesRead, 0) == -1) {
            perror("send");
            break;
        }
        usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
    }

    fclose(file);
    close(sock);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main(int argc, char *argv[]) {

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
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = "DC:A6:32:78:6D:92";  // Adresse du Raspberry Pi récepteur
    const char *file_path = "/home/PERSEUS_2023/Pole-Capteurs/client/test1.txt";

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Définir l'adresse Bluetooth de destination
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    str2ba(dest_address, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(sock, buffer, bytesRead, 0) == -1) {
            perror("send");
            break;
        }
        usleep(100000);  // Ajouter une pause pour éviter de saturer le canal Bluetooth
    }

    fclose(file);
    close(sock);

    return 0;
}
*/
//nouveau essai
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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

    printf("Bluetooth activé et en mode découvrable.\n");

    const char *file_path = argv[1];

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Utiliser n'importe quelle adresse Bluetooth locale
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) == -1) {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("En attente de connexion...\n");

    struct sockaddr_l2 client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock == -1) {
        perror("accept");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connexion établie. Réception du fichier...\n");

    char buffer[BUFFER_SIZE];
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("fopen");
        close(client_sock);
        close(sock);
        exit(EXIT_FAILURE);
    }

    ssize_t bytesRead;
    size_t total_bytes_sent = 0;
    size_t total_bytes_received = 0;
    size_t error_count = 0;

    FILE *original_file = fopen("/home/PERSEUS2023/Pole-Capteurs/client/test2.txt", "r");
    if (original_file == NULL) {
        perror("fopen (original file)");
        fclose(file);
        close(client_sock);
        close(sock);
        exit(EXIT_FAILURE);
    }

    int original_byte;

    while ((bytesRead = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesRead, file);

        // Comparer les octets reçus avec le fichier original
        for (size_t i = 0; i < bytesRead; i++) {
            total_bytes_sent++;
            total_bytes_received++;

            if (fread(&original_byte, 1, 1, original_file) != 1) {
                perror("fread (original file)");
                fclose(original_file);
                fclose(file);
                close(client_sock);
                close(sock);
                exit(EXIT_FAILURE);
            }

            if (buffer[i] != (unsigned char)original_byte) {
                error_count++;
            }
        }
    }

    fclose(original_file);
    fclose(file);
    close(client_sock);
    close(sock);

    // Calculer les taux
    double loss_rate = (double)(total_bytes_sent - total_bytes_received) / total_bytes_sent * 100.0;
    double error_rate = (double)error_count / total_bytes_sent * 100.0;

    printf("Taux de pertes : %.2f%%\n", loss_rate);
    printf("Taux d'erreurs : %.2f%%\n", error_rate);

    printf("Fichier reçu avec succès. Chemin du fichier : %s\n", file_path);

    return 0;
}

/*

//code avec comparaison taux de pertes et d'erreurs--> erreur
#include <stdio.h>
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
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    printf("bluetooth ok\n");
    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Utiliser n'importe quelle adresse Bluetooth locale
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) == -1) {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("En attente de connexion...\n");

    // Ouvrir le fichier original pour la comparaison
    FILE *original_file = fopen("/home/PERSEUS_2023/Pole-Capteurs/client/test2.txt", "r");
    if (original_file == NULL) {
        perror("fopen (original file)");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Calculer le taux de pertes et le taux d'erreurs
    size_t total_bytes_sent = 0;
    size_t total_bytes_received = 0;
    size_t error_count = 0;

    int original_char, received_char;

    while ((original_char = fgetc(original_file)) != EOF) {
        total_bytes_sent++;

        if (recv(client_sock, &received_char, sizeof(received_char), 0) != sizeof(received_char)) {
            perror("recv");
            fclose(original_file);
            close(client_sock);
            close(sock);
            exit(EXIT_FAILURE);
        }

        total_bytes_received++;

        if (original_char != received_char) {
            error_count++;
        }
    }

    fclose(original_file);

    // Calculer les taux
    double loss_rate = (double)(total_bytes_sent - total_bytes_received) / total_bytes_sent * 100.0;
    double error_rate = (double)error_count / total_bytes_sent * 100.0;

    printf("Taux de pertes : %.2f%%\n", loss_rate);
    printf("Taux d'erreurs : %.2f%%\n", error_rate);

    // Attendre la connexion
    struct sockaddr_l2 client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock == -1) {
        perror("accept");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connexion établie. Réception du fichier...\n");

    char buffer[BUFFER_SIZE];
    FILE *file = fopen("/home/PERSEUS_2023/Pole-Capteurs/client/received_file.txt", "w");
    if (file == NULL) {
        perror("fopen (received file)");
        close(client_sock);
        close(sock);
        exit(EXIT_FAILURE);
    }

    ssize_t bytesRead;

    while ((bytesRead = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);
    close(client_sock);
    close(sock);

    printf("Fichier reçu avec succès. Chemin du fichier : /home/PERSEUS_2023/Pole-Capteurs/serevr/received_file.txt\n");

    return 0;
}

/*
//"le basique"
#include <stdio.h>
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
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);
    printf("bluetooth ok\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *local_address = "DC:A6:32:78:6C:7E";  // Adresse du Raspberry Pi émetteur
    const char *file_path = argv[1];

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Utiliser n'importe quelle adresse Bluetooth locale
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) == -1) {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("En attente de connexion...\n");

    struct sockaddr_l2 client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock == -1) {
        perror("accept");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connexion établie. Réception du fichier...\n");

    char buffer[BUFFER_SIZE];
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("fopen");
        close(client_sock);
        close(sock);
        exit(EXIT_FAILURE);
    }

    ssize_t bytesRead;
    while ((bytesRead = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);
    close(client_sock);
    close(sock);

    printf("Fichier reçu avec succès. Chemin du fichier : %s\n", file_path);

    return 0;
}
/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main() {
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
    printf("bluetooth ok\n");
    
    const char *local_address = "DC:A6:32:78:6C:7E";  // Adresse du Raspberry Pi émetteur

    // Créer le socket L2CAP
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = {{0, 0, 0, 0, 0, 0}},  // Utiliser n'importe quelle adresse Bluetooth locale
        .l2_psm = htobs(0x1001),           // Canal L2CAP
        .l2_cid = 0
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) == -1) {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_l2 client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock == -1) {
        perror("accept");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    FILE *file = fopen("received_file.txt", "wb");
    if (file == NULL) {
        perror("fopen");
        close(client_sock);
        close(sock);
        exit(EXIT_FAILURE);
    }

    ssize_t bytesRead;
    while ((bytesRead = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);
    close(client_sock);
    close(sock);

    return 0;
}*/
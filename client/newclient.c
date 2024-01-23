#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main(int argc, char *argv[]) {

  
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_address = "DC:A6:32:78:6D:92";  // Adresse du Raspberry Pi récepteur
    const char *file_path = argv[1];

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



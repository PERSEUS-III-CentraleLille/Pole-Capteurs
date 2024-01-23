#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main() {
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
}

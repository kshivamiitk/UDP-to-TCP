#include "structureFiles/utils.h"
#include"structureFiles/handshake.h"
#include"structureFiles/checksum.h"
int main() {

    SOCKET client_socket;
    sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == INVALID_SOCKET) {
        log_message("CLIENT" , "Error creating socket.");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (perform_handshake(client_socket, server_addr)) {
        log_message("CLIENT" ,"Handshake successful. Connection established securely.");
        log_message("CLIENT" , "Closing connection.");
    } else {
        log_message("CLIENT" , "Handshake failed. Could not connect to server.");
    }

    closesocket(client_socket);
    return 0;
}







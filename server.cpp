#include "include/utils.h"
#include "include/handshake.h"
#include "include/checksum.h"

int main(){
    SOCKET server_socket;
    sockaddr_in server_address;

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket == INVALID_SOCKET){
        log_message("SERVER" , "Error creating the socket...");
        return 1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if(bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address))==SOCKET_ERROR){
        log_message("SERVER" , "BIND FAILED\n");
        closesocket(server_socket);
        return 1;
    }
    log_message("SERVER" , "Server is listening to the port" + std:: to_string(PORT));

    while(true){
        log_message("SERVER" , 
        "waiting for the client to initiate handshake");
        if(handle_handshake(server_socket)){
            log_message("SERVER" , "Connection established securely. Ready for data transfer.");
        }
        else{
            log_message("SERVER" , "Handshake failed. Awaiting new conection.");
        }
    }

    closesocket(server_socket);
    return 0;
}   
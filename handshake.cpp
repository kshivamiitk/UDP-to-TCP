#include "utils.h"
#include "checksum.h"


bool perform_handshake(SOCKET client_socket, sockaddr_in&server_address){
    char buffer[BUFFER_SIZE];
    TCP_Head syn_header;
    syn_header.SYN = true;
    syn_header.ACK = false;
    syn_header.checksum = 0;
    syn_header.checksum = calculate_checksum(&syn_header,
    sizeof(TCP_Head));
    log_message("CLIENT", "STEP1, sending syn to server with checksum");
    if(sendto(client_socket, reinterpret_cast<char*>(&syn_header), 
    sizeof(TCP_Head), 0 , (struct sockaddr*)&server_address, sizeof(server_address))
    ==SOCKET_ERROR){
        log_message("CLIENT", "sendto() failed." );
        return false;
    }

    socklen_t server_address_length = sizeof(server_address);
    int bytes_received = recvfrom(client_socket, buffer, 
    BUFFER_SIZE, 0 , (struct sockaddr*)&server_address,
    &server_address_length);
    if(bytes_received > 0){
        if(!verify_checksum(buffer, sizeof(TCP_Head))){
            log_message("CLIENT" , "Invalid checksum received in SYN-ACK. Handshake failed.");
            return false;
        }
        TCP_Head* header = reinterpret_cast<TCP_Head*>(buffer);
        if(header->SYN && header->ACK){
            log_message("CLIENT" , "STEP 2: Received valid SYN-ACK from server.");

            TCP_Head ack_header;
            ack_header.SYN = false;
            ack_header.ACK= true;
            ack_header.checksum = 0;
            ack_header.checksum = 
            calculate_checksum(&ack_header, sizeof(TCP_Head));

            log_message("CLIENT" ,"STEP 3: Sending ACK to server with checksum. Handshake complete.");

            if(sendto(client_socket, reinterpret_cast<char*>(&ack_header), sizeof(TCP_Head), 0 , (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR){
                log_message("CLIENT" ,"sendto() failed for ACK.");
                return false;
            }
            return true;
        }
        else{
            log_message("CLIENT" ,"Did not receive SYN-ACK from server.");
        }
        
    }

    return false;
}

bool handle_handshake(SOCKET server_socket){
    char buffer[BUFFER_SIZE];
    sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);

    int bytes_received = recvfrom(server_socket, buffer, 
    BUFFER_SIZE, 0 , (struct sockaddr*)&client_address,
    &client_address_length);

    if(bytes_received > 0){
        if(!verify_checksum(buffer, sizeof(TCP_Head))){
            log_message("SERVER" , "Invalid checksum on received SYN packet. Discarding.");
            return false;
        }
        TCP_Head *header = reinterpret_cast<TCP_Head*>(buffer);
        if(header->SYN && !header->ACK){
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_address.sin_addr,
            client_ip , INET_ADDRSTRLEN);
            log_message("SERVER" ,"STEP 1: Received valid SYN from client " + std::string(client_ip));        }

            TCP_Head syn_ack_header;
            syn_ack_header.SYN = true;
            syn_ack_header.ACK = true;
            syn_ack_header.checksum = 0;
            syn_ack_header.checksum = calculate_checksum(&syn_ack_header, sizeof(TCP_Head));

            log_message("SERVER" ,"STEP 2: Sending SYN-ACK to client with checksum.");
            sendto(server_socket, reinterpret_cast<char*>(&syn_ack_header), sizeof(TCP_Head), 0
            , (struct sockaddr*)&client_address, client_address_length);

            bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE, 0 , (struct sockaddr*)&client_address, &client_address_length);

            if(bytes_received > 0){
                if(!verify_checksum(buffer, sizeof(TCP_Head))){
                    log_message("SERVER" ,"Invalid checksum on received ACK packet. Discarding.");
                    return false;
                }
                header = reinterpret_cast<TCP_Head*>(buffer);
                if(!header->SYN && header->ACK){
                    log_message("SERVER" ,"STEP 3: Received valid ACK from client. Handshake complete.");
                    return true;
                }
            }
    }
    return false;
}

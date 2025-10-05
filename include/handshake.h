#pragma once
#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include "utils.h"
#include <netinet/in.h>

// perform client-side three-way handshake (SYN -> SYN-ACK -> ACK)
bool perform_handshake(SOCKET client_socket, sockaddr_in &server_address);

// perform server-side handshake (receive SYN, send SYN-ACK, receive ACK)
bool handle_handshake(SOCKET server_socket);

#endif // HANDSHAKE_H

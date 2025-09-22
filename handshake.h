#pragma once
#include "utils.h"

bool perform_handshake(SOCKET client_socket, sockaddr_in &server_address);
bool handle_handshake(SOCKET server_socket);
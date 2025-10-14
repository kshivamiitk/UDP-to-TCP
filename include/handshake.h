
#pragma once
#ifndef HANDSHAKE_H
#define HANDSHAKE_H
#include "utils.h"
#include "Packet.h"
bool perform_handshake(SOCKET s, const sockaddr_in& server);
bool handle_handshake(SOCKET s);
#endif

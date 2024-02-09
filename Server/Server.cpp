#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "ConnectServer.h"

// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")

int main() {
    ConnectServer Server;
    return 0;
}
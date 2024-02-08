#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include "Connect.h"

// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")


int main() {
    Connect Client;
    Client.Send("papagnan");
    return 0;
}


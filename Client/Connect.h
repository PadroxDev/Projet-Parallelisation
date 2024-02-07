#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>


class Connect
{
private:
    WSADATA wsaData;
    SOCKET ConnectSocket;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    char* recvbuf;
    int iResult;
    int recvbuflen;

public:
	Connect();
	~Connect();
    int InitializeWinSock();
    int SetConnexion();
    int CreateHiddenWindow();
	
};


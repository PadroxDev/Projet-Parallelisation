#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

class Connect
{
private:
    WSADATA wsaData;
    SOCKET ConnectSocket;
    char* recvbuf;
    int iResult;
    int recvbuflen;

public:
	Connect();
	~Connect();
    bool InitializeWinSock();
    bool CreateClientSocket(const char* serverAddress);
    bool CreateHiddenWindow(HINSTANCE hInstance, WNDPROC wndProc, HWND* pWindow);
    bool AssociateSocketWithWindow(HWND window, LONG events);
    void CleanupSocket(SOCKET socket);
    void CleanupWinsock();
    int Send(const char* buff);
    int initialize();
};
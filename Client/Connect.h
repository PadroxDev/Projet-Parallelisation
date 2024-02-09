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
    HWND hWnd;
public:
    Connect();
    ~Connect();
    bool InitializeWinSock();
    SOCKET CreateAndConnectSocket(const char* serverAddress);
    bool CreateSocket(const char* serverAddress);
    bool CreateHiddenWindow();
    bool AssociateSocketWithWindow(HWND window, LONG events);
    void CleanupSocket(SOCKET socket);
    void CleanupWinsock();
    int Send(char* buff);
    int Send(const char* buff);
    int initialize();
    static LRESULT CALLBACK ClientWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};
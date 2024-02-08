#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define DEFAULT_PORT "21"
#define DEFAULT_BUFLEN 512
#include "ConnectServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

ConnectServer::ConnectServer(){
    Initialize();
}

bool ConnectServer::InitializeWinsock() {
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }
    return true;
}

SOCKET ConnectServer::CreateListenSocket() {

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        return INVALID_SOCKET;
    }

    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);

    return listenSocket;
}

bool ConnectServer::StartListening(SOCKET listenSocket) {
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        return false;
    }
    return true;
}

bool ConnectServer::CreateHiddenWindow(HINSTANCE hInstance, WNDPROC wndProc, HWND* pWindow) {
    WNDCLASS windowClass = { 0 };
    windowClass.lpfnWndProc = wndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = L"MyWindowClass";

    if (!RegisterClass(&windowClass)) {
        printf("RegisterClass failed with error: %d\n", GetLastError());
        return false;
    }

    *pWindow = CreateWindowEx(0, L"MyWindowClass", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (*pWindow == NULL) {
        printf("CreateWindowEx failed with error: %d\n", GetLastError());
        return false;
    }

    return true;
}


bool AssociateSocketWithWindow(SOCKET socket, HWND window, LONG events) {
    if (WSAAsyncSelect(socket, window, WM_USER + 1, events) == SOCKET_ERROR) {
        printf("WSAAsyncSelect failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

void ConnectServer::CleanupSocket(SOCKET socket) {
    closesocket(socket);
}

void ConnectServer::CleanupWinsock() {
    WSACleanup();
}

int ConnectServer::Initialize() {
    if (!InitializeWinsock()) {
        return 1;
    }

    SOCKET listenSocket = CreateListenSocket();
    if (listenSocket == INVALID_SOCKET) {
        CleanupWinsock();
        return 1;
    }

    if (!StartListening(listenSocket)) {
        CleanupSocket(listenSocket);
        CleanupWinsock();
        return 1;
    }

    HWND serverWindow;
    if (!CreateHiddenWindow(GetModuleHandle(NULL), ServerWindowProc, &serverWindow)) {
        CleanupSocket(listenSocket);
        CleanupWinsock();
        return 1;
    }

    if (!AssociateSocketWithWindow(listenSocket, serverWindow, FD_ACCEPT)) {
        CleanupSocket(listenSocket);
        CleanupWinsock();
        return 1;
    }
    return 0;
}

LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_USER + 1:
        if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT) {
            // Accept a new connection
            SOCKET ListenSocket = wParam;
            SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
            clientSockets.push_back(ClientSocket);
            if (ClientSocket != INVALID_SOCKET) {
                // Associate the client socket with the window to receive messages for socket events
                WSAAsyncSelect(ClientSocket, hwnd, WM_USER + 1, FD_READ | FD_CLOSE);
            }
        }
        else if (WSAGETSELECTEVENT(lParam) == FD_READ) {
            // Handle read event
            SOCKET ClientSocket = wParam;
            char recvbuf[DEFAULT_BUFLEN];
            int bytesRead = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (bytesRead > 0) {
                //printf("Bytes received: %d\n", bytesRead);
                printf("%.*s\n", bytesRead, recvbuf);

                // Echo back the received data
                send(ClientSocket, recvbuf, bytesRead, 0);
            }
        }
        else if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
            // Handle close event
            printf("Connection closed\n");
            closesocket(wParam);
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
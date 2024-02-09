#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>

#include "ConnectServer.h"

#define DEFAULT_PORT "21"
#define DEFAULT_BUFLEN 512

ConnectServer::ConnectServer() : serverSocket(INVALID_SOCKET), hWnd(NULL) {
    Initialize();
}

ConnectServer::~ConnectServer() {
    Cleanup(serverSocket);
}

bool ConnectServer::InitializeWinsock() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }
    return true;
}

bool ConnectServer::CreateClientSocket() {
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        return false;
    }

    serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (serverSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        Cleanup();
        return false;
    }

    iResult = bind(serverSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        Cleanup(serverSocket);
        return false;
    }

    freeaddrinfo(result);
    return true;
}

bool ConnectServer::StartListening() {
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        Cleanup(serverSocket);
        return false;
    }
    return true;
}

bool ConnectServer::CreateHiddenWindow() {
    WNDCLASS windowClass = { 0 };
    windowClass.lpfnWndProc = ServerWindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.lpszClassName = L"MyWindowClass";

    if (!RegisterClass(&windowClass)) {
        printf("RegisterClass failed with error: %d\n", GetLastError());
        return false;
    }

    hWnd = CreateWindowEx(0, L"MyWindowClass", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (hWnd == NULL) {
        printf("CreateWindowEx failed with error: %d\n", GetLastError());
        return false;
    }

    return true;
}

bool ConnectServer::AssociateWithWindow() {
    LONG events = FD_ACCEPT;
    if (WSAAsyncSelect(serverSocket, hWnd, WM_USER + 1, events) == SOCKET_ERROR) {
        printf("WSAAsyncSelect failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

void ConnectServer::Cleanup(SOCKET socket = NULL) {
    if (socket) closesocket(socket);
    Cleanup();
}

bool ConnectServer::Initialize() {
    if (!InitializeWinsock())
        return true;

    serverSocket = CreateClientSocket();
    if (serverSocket == INVALID_SOCKET) {
        Cleanup();
        return true;
    }

    if (!StartListening()) {
        Cleanup(serverSocket);
        return true;
    }

    if (!CreateHiddenWindow()) {
        Cleanup(serverSocket);
        return true;
    }

    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

    if (!AssociateWithWindow()) {
        Cleanup(serverSocket);
        return true;
    }
    return false;
}

void ConnectServer::HandleAccept(SOCKET sock) {
    SOCKET incomingSocket;
    incomingSocket = accept(sock, NULL, NULL);
    if (incomingSocket == INVALID_SOCKET) {
        Cleanup(serverSocket);
        std::cout << "Error accepting an incomming socket !" << std::endl;
        return;
    }
    clientSockets.push_back(incomingSocket);
    WSAAsyncSelect(incomingSocket, hWnd, WM_USER + 1, FD_READ | FD_CLOSE);
}

void ConnectServer::HandleRead(SOCKET sock) {
    char recvbuf[DEFAULT_BUFLEN];
    int bytesRead = recv(sock, recvbuf, DEFAULT_BUFLEN, 0);
    if (bytesRead > 0) {
        //printf("Bytes received: %d\n", bytesRead);
        printf("%.*s\n", bytesRead, recvbuf);

        // Echo back the received data
        send(sock, recvbuf, bytesRead, 0);
    }
}

void ConnectServer::HandleClose(SOCKET sock) {
    std::cout << "Connection closed" << std::endl;
    for (int i = clientSockets.size() - 1; i >= 0; i--)
    {
        if (clientSockets[i] == sock) {
            clientSockets.erase(clientSockets.begin() + i);
            break;
        }
    }
    closesocket(sock);
}

void ConnectServer::EventDispatcher(int fdEvent, SOCKET sock) {
    switch (fdEvent) {
    case FD_ACCEPT:
        HandleAccept(sock);
        break;
    case FD_READ:
        HandleRead(sock);
        break;
    case FD_CLOSE:
        HandleClose(sock);
        break;
    default:
        std::cout << "Event not found: " << fdEvent << " !" << std::endl;
        break;
    }
}

LRESULT CALLBACK ConnectServer::ServerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ConnectServer* pServer = (ConnectServer*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_USER + 1:
        int fdEvent = WSAGETSELECTEVENT(lParam);
        SOCKET sock = wParam; // Socket client qui fait la requête

        pServer->EventDispatcher(fdEvent, sock);

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
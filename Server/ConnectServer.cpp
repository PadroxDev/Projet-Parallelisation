#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "ConnectServer.h"

#define DEFAULT_PORT "21"
#define DEFAULT_BUFLEN 512

ConnectServer::ConnectServer() : clientSocket(INVALID_SOCKET), hWnd(NULL) {
    Initialize();
}

ConnectServer::~ConnectServer()
{}

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

    clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        Cleanup();
        return false;
    }

    iResult = bind(clientSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        Cleanup(clientSocket);
        return false;
    }

    freeaddrinfo(result);
    return true;
}

bool ConnectServer::StartListening() {
    if (listen(clientSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        Cleanup(clientSocket);
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
    if (WSAAsyncSelect(clientSocket, hWnd, WM_USER + 1, events) == SOCKET_ERROR) {
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

    clientSocket = CreateClientSocket();
    if (clientSocket == INVALID_SOCKET) {
        Cleanup();
        return true;
    }

    if (!StartListening()) {
        Cleanup(clientSocket);
        return true;
    }

    if (!CreateHiddenWindow()) {
        Cleanup(clientSocket);
        return true;
    }

    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

    if (!AssociateWithWindow()) {
        Cleanup(clientSocket);
        return true;
    }
    return false;
}

LRESULT CALLBACK ConnectServer::ServerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    ConnectServer* pServer = (ConnectServer*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_USER + 1:
        if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT) {
            // Accept a new connection
            SOCKET ListenSocket = wParam;
            SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
            pServer->clientSockets.push_back(ClientSocket);
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
            Cleanup(clientSocket);
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
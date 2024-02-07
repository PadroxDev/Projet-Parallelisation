#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")

#define DEFAULT_PORT "21"
#define DEFAULT_BUFLEN 512

LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int main() {
    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        hints;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    struct addrinfo* ptr = NULL;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the socket to an address and port
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Start listening on the socket
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Create a hidden window for socket events
    WNDCLASS serverWindowClass = { 0 };
    serverWindowClass.lpfnWndProc = ServerWindowProc;
    serverWindowClass.hInstance = GetModuleHandle(NULL);
    serverWindowClass.lpszClassName = L"MyServerWindowClass";
    RegisterClass(&serverWindowClass);

    HWND serverWindow = CreateWindowEx(0, L"MyServerWindowClass", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (serverWindow == NULL) {
        printf("CreateWindowEx failed with error: %d\n", GetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Associate the socket with the window to receive messages for socket events
    WSAAsyncSelect(ListenSocket, serverWindow, WM_USER + 1, FD_ACCEPT);

    // Receive until the peer closes the connection
    MSG msg;
    while (GetMessage(&msg, serverWindow, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // cleanup
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}

LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_USER + 1:
        if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT) {
            // Accept a new connection
            SOCKET ListenSocket = wParam;
            SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
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
                printf("Bytes received: %d\n", bytesRead);
                printf("Data received: %.*s\n", bytesRead, recvbuf);

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
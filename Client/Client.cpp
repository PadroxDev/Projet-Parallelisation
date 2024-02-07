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

LRESULT CALLBACK ClientWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
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

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("10.1.144.29", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Create a hidden window for socket events
    WNDCLASS clientWindowClass = { 0 };
    clientWindowClass.lpfnWndProc = ClientWindowProc;
    clientWindowClass.hInstance = GetModuleHandle(NULL);
    clientWindowClass.lpszClassName = L"MyClientWindowClass";
    RegisterClass(&clientWindowClass);

    HWND clientWindow = CreateWindowEx(0, L"MyClientWindowClass", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (clientWindow == NULL) {
        printf("CreateWindowEx failed with error: %d\n", GetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Associate the socket with the window to receive messages for socket events
    WSAAsyncSelect(ConnectSocket, clientWindow, WM_USER + 1, FD_READ | FD_CLOSE);

    // Send an initial buffer
    iResult = send(ConnectSocket, "this is a test", 14, 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    printf("Bytes Sent: %ld\n", iResult);

    // Receive until the peer closes the connection
    do {
        MSG msg;
        if (GetMessage(&msg, clientWindow, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } while (iResult > 0);

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

LRESULT CALLBACK ClientWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_USER + 1:
        if (WSAGETSELECTEVENT(lParam) == FD_READ) {
            // Handle read event
            SOCKET sock = wParam;
            char buffer[DEFAULT_BUFLEN];
            int bytesRead = recv(sock, buffer, DEFAULT_BUFLEN, 0);
            if (bytesRead > 0) {
                printf("Bytes received: %d\n", bytesRead);
                printf("Data received: %s\n", buffer);
            }
        }
        else if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
            // Handle close event
            printf("Connection closed\n");
            closesocket(wParam);
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
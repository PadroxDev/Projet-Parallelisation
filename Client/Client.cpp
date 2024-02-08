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

bool InitializeWinsock() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }
    return true;
}

SOCKET CreateAndConnectSocket(const char* serverAddress) {
    struct addrinfo* result = nullptr, * ptr = nullptr, hints;
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(serverAddress, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        return INVALID_SOCKET;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed: %ld\n", WSAGetLastError());
            continue;
        }

        // Connect to server.
        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        return INVALID_SOCKET;
    }

    return connectSocket;
}

bool CreateHiddenWindow(HINSTANCE hInstance, WNDPROC wndProc, HWND* pWindow) {
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

void CleanupSocket(SOCKET socket) {
    closesocket(socket);
}

void CleanupWinsock() {
    WSACleanup();
}

int main() {
    if (!InitializeWinsock()) {
        return 1;
    }

    SOCKET connectSocket = CreateAndConnectSocket("10.1.144.29");
    if (connectSocket == INVALID_SOCKET) {
        CleanupWinsock();
        return 1;
    }

    HWND clientWindow;
    if (!CreateHiddenWindow(GetModuleHandle(NULL), ClientWindowProc, &clientWindow)) {
        CleanupSocket(connectSocket);
        CleanupWinsock();
        return 1;
    }

    if (!AssociateSocketWithWindow(connectSocket, clientWindow, FD_READ | FD_CLOSE)) {
        CleanupSocket(connectSocket);
        CleanupWinsock();
        return 1;
    }

    // Send an initial buffer
    while (true)
    {
        int iResult = send(connectSocket, "----------------------------\n", 14, 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
            CleanupSocket(connectSocket);
            CleanupWinsock();
            return 1;
        }
        printf("Bytes Sent: %d\n", iResult);
    }

    // Receive until the peer closes the connection
    MSG msg;
    while (GetMessage(&msg, clientWindow, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupSocket(connectSocket);
    CleanupWinsock();

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

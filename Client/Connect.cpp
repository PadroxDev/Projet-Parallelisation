#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Connect.h"
#include <stdio.h>
#include <stdlib.h>
#define DEFAULT_PORT "21"
#define DEFAULT_BUFLEN 512

const char* SERVER_IP_ADDR = "10.1.144.30";

Connect::Connect() : ConnectSocket(INVALID_SOCKET) {
    recvbuf[DEFAULT_BUFLEN];
    recvbuflen = DEFAULT_BUFLEN;
    initialize();
};

Connect::~Connect() {
    CleanupSocket(ConnectSocket);
    CleanupWinsock();
};

bool Connect::InitializeWinSock() {
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }
    return true;
};

SOCKET Connect::CreateAndConnectSocket(const char* serverAddress) {

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
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed: %ld\n", WSAGetLastError());
            continue;
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
        return INVALID_SOCKET;
    }

    return ConnectSocket;
}

bool Connect::CreateHiddenWindow() {
    WNDCLASS clientWindowClass = { 0 };
    clientWindowClass.lpfnWndProc = ClientWindowProc;
    clientWindowClass.hInstance = GetModuleHandle(NULL);
    clientWindowClass.lpszClassName = L"MyClientWindowClass";

    if (clientWindowClass.hInstance == NULL) {
        printf("GetModuleHandle failed with error: %d\n", GetLastError());
        return false;
    }

    if (!RegisterClass(&clientWindowClass)) {
        printf("RegisterClass failed with error: %d\n", GetLastError());
        return false;
    }

    hWnd = CreateWindowEx(0, L"MyClientWindowClass", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, clientWindowClass.hInstance, NULL);
    if (hWnd == NULL) {
        printf("CreateWindowEx failed with error: %d\n", GetLastError());
        return false;
    }
    return true;
}

bool Connect::AssociateSocketWithWindow(HWND window, LONG events) {
    if (WSAAsyncSelect(ConnectSocket, window, WM_USER + 1, events) == SOCKET_ERROR) {
        printf("WSAAsyncSelect failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

void Connect::CleanupSocket(SOCKET socket) {
    closesocket(socket);
}

void Connect::CleanupWinsock() {
    WSACleanup();
}

int Connect::initialize() {
    if (!InitializeWinSock()) {
        return 1;
    }
    CreateAndConnectSocket(SERVER_IP_ADDR);
    if (ConnectSocket == INVALID_SOCKET) {
        CleanupWinsock();
        return 1;
    }
    if (!CreateHiddenWindow()){
        CleanupSocket(ConnectSocket);
        CleanupWinsock();
        return 1;
    }
    if (!AssociateSocketWithWindow(hWnd, FD_READ | FD_CLOSE)) {
        CleanupSocket(ConnectSocket);
        CleanupWinsock();
        return 1;
    }
    return 0;
}

int Connect::Send(const char* buff) {
    iResult = send(ConnectSocket,buff,strlen(buff), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        CleanupSocket(ConnectSocket);
        CleanupWinsock();
        return 1;
    }
    printf("Bytes Sent: %d\n", iResult);
    return 0;
}

LRESULT CALLBACK Connect::ClientWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    Connect* pClient = (Connect*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_USER + 1:

        if (WSAGETSELECTEVENT(lParam) == FD_READ) {
            SOCKET sock = wParam;
            char buffer[DEFAULT_BUFLEN];
            int bytesRead = recv(sock, buffer, DEFAULT_BUFLEN, 0);
            if (bytesRead > 0) {
                printf("Bytes received: %d\n", bytesRead);
                printf("Data received: %s\n", buffer);
            }
        }
        else if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
            printf("Connection closed\n");
            closesocket(wParam);
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
        break;
    }
    return 0;
}

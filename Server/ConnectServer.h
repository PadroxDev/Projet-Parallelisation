#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>


class ConnectServer
{

public:

	
	WSADATA wsaData;
	int iResult;
	std::vector<SOCKET> clientSockets;
	struct addrinfo* result = nullptr, * ptr = nullptr, hints;
	SOCKET listenSocket = INVALID_SOCKET;
	ConnectServer();
	bool InitializeWinsock();
	SOCKET CreateListenSocket();
	bool StartListening(SOCKET listenSocket);
	bool CreateHiddenWindow(HINSTANCE hInstance, WNDPROC wndProc, HWND* pWindow);
	bool AssociateSocketWithWindow(SOCKET socket, HWND window, LONG events);
	void CleanupSocket(SOCKET socket);
	void CleanupWinsock();
	int Initialize();
};
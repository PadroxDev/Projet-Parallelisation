#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>


class ConnectServer
{
private:
	SOCKET clientSocket;
	HWND hWnd;
	std::vector<SOCKET> clientSockets;
public:
	ConnectServer();
	~ConnectServer();

	bool Initialize();
	bool InitializeWinsock();
	bool CreateClientSocket();
	bool StartListening();
	bool CreateHiddenWindow();
	bool AssociateWithWindow();
	void Cleanup(SOCKET socket);
	
	static LRESULT CALLBACK ServerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <string>
#include <json/json.h>
#include <iostream>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "21"

std::map<std::string, int> requests = {
    {"post", 1},
};

std::string convertJsonToString(const Json::Value& json) {
    Json::StreamWriterBuilder builder;
    std::ostringstream os;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(json, &os);
    return os.str();
}

Json::Value parseJsonFromString(const std::string& jsonString) {
    Json::CharReaderBuilder builder;
    Json::Value jsonData;
    std::string errs;
    std::istringstream is(jsonString);
    if (!Json::parseFromStream(builder, is, &jsonData, &errs)) {
        std::cerr << "Erreur lors du parsing JSON : " << errs << std::endl;
        // Handle Error
    }
    return jsonData;
}

int parseRequest(Json::Value json) {
    //std::string request = "";
    //int i = 0;
    //while (buffer[i] != ' ')
    //    request.push_back(buffer[i++]);
    //printf("Searching for: %s\n", request.c_str());

    //return requests[request];
    return 0;
}

int __cdecl main(void)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    printf("%s", "Client socket accepted !\n");

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    do {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Receive length: %d\n", recvbuflen);
            std::string request(recvbuf, recvbuflen);

            printf("%s\n", request.c_str());

            switch (0) {
            case 0:
                break;
            default:
                printf("Osef ???");
                break;
            }

            //iSendResult = send(ClientSocket, recvbuf, iResult, 0);
            //if (iSendResult == SOCKET_ERROR) {
            //    printf("send failed with error: %d\n", WSAGetLastError());
            //    closesocket(ClientSocket);
            //    WSACleanup();
            //    return 1;
            //}
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
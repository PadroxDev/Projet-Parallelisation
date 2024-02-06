#include <iostream>
#ifndef UNICODE
#define UNICODE 1
#endif

// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>   // Needed for _wtoi
#include <json/json.h>

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

int main(int argc, wchar_t** argv) {

    //-----------------------------------------
   // Declare and initialize variables
    WSADATA wsaData = { 0 };
    int iResult = 0;

    //    int i = 1;

    SOCKET sock = INVALID_SOCKET;
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;

    // Validate the parameters
   // if (argc != 4) {
    //    wprintf(L"usage: %s <addressfamily> <type> <protocol>\n", argv[0]);
   //     wprintf(L"socket opens a socket for the specified family, type, & protocol\n");
   //     wprintf(L"%ws example usage\n", argv[0]);
   //     wprintf(L"   %ws 0 2 17\n", argv[0]);
   //     wprintf(L"   where AF_UNSPEC=0 SOCK_DGRAM=2 IPPROTO_UDP=17\n", argv[0]);
    //    return 1;
  //  }

    //iFamily = _wtoi(argv[1]);
    //iType = _wtoi(argv[2]);
    //iProtocol = _wtoi(argv[3]);

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(21);

    // Adresse IP manuelle (remplacez par l'adresse IP réelle du serveur)
    inet_pton(AF_INET, "10.1.144.30", &(serverAddress.sin_addr));

    sock = socket(iFamily, iType, iProtocol);
    if (sock == INVALID_SOCKET) {
        printf("socket function failed with error = %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Connection Failed with error = %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    Json::Value data;
    data["command"] = "post";
    data["grid_row_0"] = "00o";
    data["grid_row_1"] = "0ox";
    data["grid_row_2"] = "xxo";
    std::string serializedData = convertJsonToString(data);

    printf("Sent Length: %d\n", strlen(serializedData.c_str()));
    send(sock, serializedData.c_str(), strlen(serializedData.c_str()), 0);
    // Lecture et affichage de la réponse du serveur
    char buffer[1024];
    int bytesRead;

    while ((bytesRead = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        // Affichez les données reçues
        fwrite(buffer, 1, bytesRead, stdout);
    }

    // Ferme la socket après avoir terminé toutes les opérations
    closesocket(sock);

    WSACleanup();

    return 0;
}
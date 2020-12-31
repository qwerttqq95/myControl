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

class my {
public:
    my();

    void mywrite(std::string);

    void myread();

    void UDPServer();

    std::string UDPClient(char*);

    HANDLE hCom;
    BYTE lpOutBuffer[1500] = {0};
    DWORD nApduLen;
    char sendBuf_server[1000];
    SOCKADDR_IN addrClient_server;   //用来接收客户端的地址信息
    SOCKET sockSrv_server;
    int len_server;
};

#endif //UNTITLED1_MYH_H

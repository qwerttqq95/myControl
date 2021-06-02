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
#include "mutex"
#include "deque"

std::mutex mtx_UDPClientToServer;
std::mutex mtx_UDPServerToClient;
std::mutex mtx_SerialToUDPServer;

std::deque<std::string> UDPClientToServer;
std::deque<std::string> UDPServerToClient;
std::deque<std::string> SerialToUDPServer;


class MultiSerial {
public:
    MultiSerial(const std::string &, const std::string &);

    void MuliWrite(std::string);

    static void MultiRead(MultiSerial *);

    static void WriteWhile(MultiSerial *);

    HANDLE MultihCom;
    std::string MultiNum;
    DWORD nApduLen;
    std::deque<std::string> WriteQue;


};


class UDP_Server {
public:
    UDP_Server();

    void CheckFromToUDPServer();

    void CheckFromUDPClient();

    char sendBuf_server[1000];
    SOCKET sockSrv_server;
    int len_server;
    SOCKADDR_IN addrClient_server;
};

class UDP_Client {
public:
    UDP_Client();

    void rec(SOCKET sockSrv,sockaddr_in addrSrv);
};


#endif //UNTITLED1_MYH_H

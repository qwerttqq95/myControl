#include <WinSock2.h>
#include <iostream>
#include <string>
#include <cstring>
#include "vector"


using namespace std;

#pragma comment(lib, "Ws2_32.lib")

int main() {
    //加载套接字库
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(1, 1);

    err = WSAStartup(wVersionRequested, &wsaData);//错误会返回WSASYSNOTREADY
    if (err != 0) {
        cout << "1";
        return 0;
    }

    if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
        HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
    {
        cout << "2";
        WSACleanup();
        return 0;
    }

    printf("server is operating!\n\n");
    //创建用于监听的套接字
    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);//失败会返回 INVALID_SOCKET
    printf("Failed. Error Code : %d",WSAGetLastError());//显示错误信息

    SOCKADDR_IN addrSrv;     //定义sockSrv发送和接收数据包的地址
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(10001);

    //绑定套接字, 绑定到端口
    bind(sockSrv, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));//会返回一个SOCKET_ERROR
    //将套接字设为监听模式， 准备接收客户请求


    SOCKADDR_IN addrClient;   //用来接收客户端的地址信息
    int len = sizeof(SOCKADDR);
//    char recvBuf[1000];    //收
//    char sendBuf[1000];    //发
//    char tempBuf[1000];    //存储中间信息数据
    char a = 0x0d;
    char b = 0x0a;
    string s;
    s.push_back(a);
    s.push_back(b);
    while (1) {
        char recvBuf[1000] = {0};    //收
        char sendBuf[1000];    //发
        //等待并数据
        recvfrom(sockSrv, recvBuf, 100, 0, (SOCKADDR *) &addrClient, &len);
        if ('q' == recvBuf[0]) {
            sendto(sockSrv, "q", strlen("q") + 1, 0, (SOCKADDR *) &addrClient, len);
            printf("Chat end!\n");
            break;
        }
        printf("receive %s\n", recvBuf);

        if (strcmp(recvBuf, "cmd=0101,data=127.0.0.1;10001") == 0) {
            strcpy(sendBuf, ("cmd=0101,ret=0,data=null"));
            strcat(sendBuf, s.data());
            sendto(sockSrv, sendBuf, strlen(sendBuf), 0, (SOCKADDR *) &addrClient, len);
            printf("sent %s\n", sendBuf);
            continue;
        }
        if (strcmp(recvBuf, "cmd=0301,data=null") == 0) {
            strcpy(sendBuf, "cmd=0301,ret=0,data=220.064;0;220.091;120;220.08;240;0;0;0;0;0;0;0;0;0;49.9992");
            strcat(sendBuf, s.data());
            sendto(sockSrv, sendBuf, strlen(sendBuf), 0, (SOCKADDR *) &addrClient, len);
            printf("sent %s\n", sendBuf);
            continue;
        }
        if ((strcmp(recvBuf, "cmd=0104,data=1;2;1;2400-e-8-1") == 0) ||
            (strcmp(recvBuf, "cmd=0104,data=0;4;1;9600-e-8-1") == 0) ||
            (strcmp(recvBuf, "cmd=0104,data=0;2;1;9600-e-8-1") == 0)) {
            strcpy(sendBuf, "cmd=0104,ret=0,data=null");
            strcat(sendBuf, s.data());
            sendto(sockSrv, sendBuf, strlen(sendBuf) + 1, 0, (SOCKADDR *) &addrClient, len);
            printf("sent %s\n", sendBuf);
            continue;
        }
        if ((strcmp(recvBuf, "cmd=0401,data=1;1;01") == 0))
        {
            strcpy(sendBuf, "cmd=0401,ret=0,data=null");
            strcat(sendBuf, s.data());
            sendto(sockSrv, sendBuf, strlen(sendBuf) + 1, 0, (SOCKADDR *) &addrClient, len);
            printf("sent %s\n", sendBuf);
            continue;
        }
        string receive = recvBuf;
        string::size_type position;
        position = receive.find("68");
        if (position != std::string::npos)  //如果没找到，返回一个特别的标志c++中用npos表示，我这里npos取值是4294967295，
        {
            printf("position is : %d\n", position);
            string sub = receive.substr(position,receive.length()-1);//698报文


        } else {
            printf("Not found the flag\n");
        }
        if (strcmp(recvBuf,"cmd=1001,data=1;0;6817004345AAAAAAAAAAAA10DA5F050100F100020000964A16")==0)
        {
            strcpy(sendBuf, "cmd=1001,ret=0,data=1;682200C30506060200010000CD29850100F10002000109074C5931303236360000CE0616");
            strcat(sendBuf, s.data());
            sendto(sockSrv, sendBuf, strlen(sendBuf), 0, (SOCKADDR *) &addrClient, len);
            printf("sent %s\n", sendBuf);
            continue;
        }
    }
}

//
// Created by Admin on 2020/12/30.
//


//string serial(string text) {
//    HANDLE hCom; //全局变量，串口句柄
//    hCom = CreateFile("\\\\.\\COM18",
//                      GENERIC_READ | GENERIC_WRITE, //允许读和写
//                      0, //独占方式
//                      NULL,
//                      OPEN_EXISTING, //打开而不是创建
//                      0, //同步方式
//                      NULL);
//    if (hCom == (HANDLE) -1) {
//        cout << "open fail!";
//        return "";
//    }
//    BYTE lpOutBuffer[1500] = {0};
//    DWORD nApduLen = text.length() / 2;
//    StringToHex(text, lpOutBuffer);
//    SetupComm(hCom, 1024, 1024); //输入缓冲区和输出缓冲区的大小都是1024
//    COMMTIMEOUTS TimeOuts; //设定读超时
//    TimeOuts.ReadIntervalTimeout = 2000;
//    TimeOuts.ReadTotalTimeoutMultiplier = 2000;
//    TimeOuts.ReadTotalTimeoutConstant = 2000; //设定写超时
//    TimeOuts.WriteTotalTimeoutMultiplier = 500;
//    TimeOuts.WriteTotalTimeoutConstant = 500;
//    SetCommTimeouts(hCom, &TimeOuts); //设置超时
//    DCB dcb;
//    GetCommState(hCom, &dcb);
//    dcb.BaudRate = 9600;
//    dcb.ByteSize = 8;
//    dcb.Parity = EVENPARITY;
//    dcb.StopBits = ONESTOPBIT;
//    SetCommState(hCom, &dcb);
//    PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);
//
//
////    char lpOutBuffer[100] = {0x68};
////    DWORD dwBytesWrite = 1000;
//    COMSTAT ComStat;
//    DWORD dwErrorFlags;
//    BOOL bWriteStat;
//    ClearCommError(hCom, &dwErrorFlags, &ComStat);
//    bWriteStat = WriteFile(hCom, lpOutBuffer, nApduLen, &nApduLen, NULL);
//    if (!bWriteStat) { cout << "写串口失败!"; }
////    PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
//
//    char str[1000];
//    DWORD wCount;//读取的字节数
//    BOOL bReadStat;
//    cout << "Reading...\n";
//    bReadStat = ReadFile(hCom, str, 200, &wCount, NULL);
//    if (!bReadStat) {
//        cout << "读串口失败!";
//        return "";
//    }
//    string output;
//    char temp[4] = {0};
//    for (int i = 0; i < wCount; i++) {
//        sprintf(temp, "%02X", (BYTE) str[i]);
//        output = output + temp;
//    }
//    CloseHandle(hCom);
//    return output;
//}
//
//string UDPClient(char sendBuf[1000]) {
//    //加载套接字库
//    WORD wVersionRequested;
//    WSADATA wsaData;
//    int err;
//    char recvBuf[1000] = {0};    //收
//
//    wVersionRequested = MAKEWORD(1, 1);
//
//    err = WSAStartup(wVersionRequested, &wsaData);
//    if (err != 0) {
//        return "";
//    }
//
//    if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
//        HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
//    {
//        WSACleanup();
//        return "";
//    }
//
////    printf("Client is operating!\n\n");
//    //创建用于监听的套接字
//    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);
//
//    sockaddr_in addrSrv;
//    addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
//    addrSrv.sin_family = AF_INET;
//    addrSrv.sin_port = htons(10001);
//
//    int len = sizeof(SOCKADDR);
//
////    string sendBufs = "cmd=0101,data=127.0.0.1;10001";
////    strncpy(sendBuf, sendBufs.c_str(), sendBufs.length() + 1); // 注意，一定要加1，否则没有赋值'\0'
//
//    sendto(sockSrv, sendBuf, strlen(sendBuf) + 1, 0, (SOCKADDR *) &addrSrv, len);
//    //等待并数据
//    recvfrom(sockSrv, recvBuf, 1000, 0, (SOCKADDR *) &addrSrv, &len);
//
//    if ('q' == recvBuf[0]) {
//        sendto(sockSrv, "q", strlen("q") + 1, 0, (SOCKADDR *) &addrSrv, len);
//        printf("Chat end!\n");
//    }
////    printf("client receive %s\n", recvBuf);
//
//    string s = recvBuf;
//    //发送数据
//    return s;
//
//}
//
//void UDPServer() {
//    WORD wVersionRequested;
//    WSADATA wsaData;
//    int err;
//    wVersionRequested = MAKEWORD(1, 1);
//    err = WSAStartup(wVersionRequested, &wsaData);//错误会返回WSASYSNOTREADY
//    if (err != 0) {
//        cout << "1";
//        return;
//    }
//
//    if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
//        HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
//    {
//        cout << "2";
//        WSACleanup();
//        return;
//    }
//
//    printf("server is operating!\n\n");
//    //创建用于监听的套接字
//    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);//失败会返回 INVALID_SOCKET
////    printf("Failed. Error Code : %d", WSAGetLastError());//显示错误信息
//
//    SOCKADDR_IN addrSrv;     //定义sockSrv发送和接收数据包的地址
//    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//    addrSrv.sin_family = AF_INET;
//    addrSrv.sin_port = htons(10002);
//
//    //绑定套接字, 绑定到端口
//    bind(sockSrv, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));//会返回一个SOCKET_ERROR
//    //将套接字设为监听模式， 准备接收客户请求
//
//
//    SOCKADDR_IN addrClient;   //用来接收客户端的地址信息
//    int len = sizeof(SOCKADDR);
////    char recvBuf[1000];    //收
////    char sendBuf[1000];    //发
////    char tempBuf[1000];    //存储中间信息数据
//    char a = 0x0d;
//    char b = 0x0a;
//    string s;
//    s.push_back(a);
//    s.push_back(b);
//    string rec;
//    while (1) {
//        char recvBuf[1000] = {0};
//        char sendBuf[1000];
//        //等待并数据
//        recvfrom(sockSrv, recvBuf, 1000, 0, (SOCKADDR *) &addrClient, &len);
//        if ('q' == recvBuf[0]) {
//            sendto(sockSrv, "q", strlen("q") + 1, 0, (SOCKADDR *) &addrClient, len);
//            printf("Chat end!\n");
//            break;
//        }
//        printf("receive %s\n", recvBuf);
//        string recvBuf_s = recvBuf;
//
//        if (recvBuf_s.find("cmd=1001,data=1;0;") == 0) {
//            cout << "serial" << endl;
//            rec = "cmd=1001,ret=0,data=1;";
//            string cut = recvBuf_s.substr(18);
////            cout<<"cut "<<cut<<endl;
//            rec += serial(cut) + s;
//            cout << "serial rec " << rec << endl;
//        } else {
//            if (recvBuf_s.find("cmd=0104") == 0) {
//                rec = "cmd=0104,ret=0,data=null" + s;
//                cout << "UDPClient rec(fake) " << rec << endl;
//            } else {
//                cout << "UDPClient" << endl;
//                rec = UDPClient(recvBuf) + s;
//                cout << "UDPClient rec " << rec << endl;
//            }
//        }
//        strncpy(sendBuf, rec.c_str(), rec.length() + 1);
//        sendto(sockSrv, sendBuf, strlen(sendBuf), 0, (SOCKADDR *) &addrClient, len);
//    }
//
//}
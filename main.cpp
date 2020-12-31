
#include "myH.h"
#include <iostream>
#include <thread>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

void StringToHex(string str, unsigned char *buff) {
    int j = 0;
    int z = 0;
    for (int i = 0; i < (str.length() / 2); i++) {
        string temp = str.substr(z, 2);
        buff[j++] = stoi(temp, 0, 16);
        z += 2;
    }
}

void ti() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    cout << ltm->tm_hour << ":";
    cout << ltm->tm_min << ":";
    cout << ltm->tm_sec << "\n";
}

my::my() {
    cout << "init" << endl;
    hCom = CreateFile("\\\\.\\COM18",
                      GENERIC_READ | GENERIC_WRITE, //允许读和写
                      0, //独占方式
                      NULL,
                      OPEN_EXISTING, //打开而不是创建
                      0, //同步方式
                      NULL);
    if (hCom == (HANDLE) -1) {
        cout << "open fail!";
        return;
    }
    SetupComm(hCom, 2048, 2048); //输入缓冲区和输出缓冲区的大小都是1024
    COMMTIMEOUTS TimeOuts; //设定读超时
    TimeOuts.ReadIntervalTimeout = MAXDWORD;
    TimeOuts.ReadTotalTimeoutMultiplier = 0;
    TimeOuts.ReadTotalTimeoutConstant = 0;
    TimeOuts.WriteTotalTimeoutMultiplier = 500;
    TimeOuts.WriteTotalTimeoutConstant = 500;
    SetCommTimeouts(hCom, &TimeOuts); //设置超时
    DCB dcb;
    GetCommState(hCom, &dcb);
    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity = EVENPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(hCom, &dcb);
    PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);

    cout << "opened serial" << endl;
    UDPServer();


}

void my::mywrite(std::string text) {
    nApduLen = text.length() / 2;
    StringToHex(text, lpOutBuffer);
    COMSTAT ComStat;
    DWORD dwErrorFlags;
    BOOL bWriteStat;
    ClearCommError(hCom, &dwErrorFlags, &ComStat);
    bWriteStat = WriteFile(hCom, lpOutBuffer, nApduLen, &nApduLen, NULL);
    if (!bWriteStat) { cout << "写串口失败!"; }
    lpOutBuffer[0] = {0};
}

void my::myread() {
    char str[1000];
    DWORD wCount;//读取的字节数
    BOOL bReadStat;
    string output;
    vector<string> map;
    char a = 0x0d;
    char b = 0x0a;
    string s;
    s.push_back(a);
    s.push_back(b);
    while (1) {
        bReadStat = ReadFile(hCom, str, 500, &wCount, NULL);
        if (!bReadStat) {
            cout << "读串口失败!";
            return;
        }
        if (wCount > 0) {
            char temp[4] = {0};
            for (int i = 0; i < wCount; i++) {
                sprintf(temp, "%02X", (BYTE) str[i]);
//                output += temp;
                map.push_back(temp);
            }
            if (map.size() > 15) {
                if (map[0] == "68") {
                    //698
                    if (map[14] == "85" and map[15] == "01") {
                        if (36 > map.size())
                            continue;
                        for (int i = 0; i < 36; ++i) {
                            output += map[i];
                        }
//                        cout << output << endl;

                        string rec = "cmd=1001,ret=0,data=1;" + output + s;
                        thread t3(&my::FuckTheCJ,this,rec);
                        t3.detach();
//                        strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
//                        for (int i = 0; i < 10; ++i) {
//                            sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
//                                   len_server);
//                            cout << "serial rec:" << rec << endl;
//                        }

                        ti();
                        map.erase(begin(map), begin(map) + 24);
                        output.clear();
                    }
                    //376.2
                    else {
                        cout << "m1:" << map[1] << endl;
                        cout << "m2:" << map[2] << endl;
                        int len = (stoi(map[1], 0, 16)) + (stoi(map[2], 0, 16) << 8);
                        cout << "len:" << len << endl;
                        if (len>300)
                        {
                            cout<<"len to long !! erase:"<<map[0]<<endl;
                            map.erase(begin(map));
                        }
                        if (len > map.size())
                            continue;
                        for (int i = 0; i < len; ++i) {
                            output += map[i];
                        }
//                        cout << output << endl;
                        string rec = "cmd=1001,ret=0,data=1;" + output + s;
                        strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
                        sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
                               len_server);
                        cout << "serial rec:" << rec << endl;
                        ti();
                        map.erase(begin(map), begin(map) + len - 1);output.clear();
                    }

                } else {
                    while (map.size() > 10 && map[0] != "68") {
                        cout<<"erase:"<<map[0]<<endl;
                        map.erase(begin(map));
                    }

                }
            }


        }

    }

}

void my::UDPServer() {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(1, 1);
    err = WSAStartup(wVersionRequested, &wsaData);//错误会返回WSASYSNOTREADY
    if (err != 0) {
        cout << "1---------------";
        return;
    }

    if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
        HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
    {
        cout << "2------------------";
        WSACleanup();
        return;
    }

    printf("server is operating!\n\n");
    //创建用于监听的套接字
    sockSrv_server = socket(AF_INET, SOCK_DGRAM, 0);//失败会返回 INVALID_SOCKET
//    printf("Failed. Error Code : %d", WSAGetLastError());//显示错误信息

    SOCKADDR_IN addrSrv;     //定义sockSrv发送和接收数据包的地址
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(10002);

    //绑定套接字, 绑定到端口
    bind(sockSrv_server, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));//会返回一个SOCKET_ERROR
    //将套接字设为监听模式， 准备接收客户请求

    len_server = sizeof(SOCKADDR);
//    char recvBuf[1000];    //收
//    char sendBuf[1000];    //发
//    char tempBuf[1000];    //存储中间信息数据
    char a = 0x0d;
    char b = 0x0a;
    string s;
    s.push_back(a);
    s.push_back(b);
    string rec;
    std::thread t(&my::myread, this);
    while (1) {
        char recvBuf[1000] = {0};
        //等待并数据
        recvfrom(sockSrv_server, recvBuf, 1000, 0, (SOCKADDR *) &addrClient_server, &len_server);
        if ('q' == recvBuf[0]) {
            sendto(sockSrv_server, "q", strlen("q") + 1, 0, (SOCKADDR *) &addrClient_server, len_server);
            printf("Chat end!\n");
            break;
        }
        printf("receive %s\n", recvBuf);
        string recvBuf_s = recvBuf;

        if (recvBuf_s.find("cmd=1001,data=1;0;") == 0) {
            cout << "into serial" << endl;
            string cut = recvBuf_s.substr(18);
            this->mywrite(cut);
        } else {
            if (recvBuf_s.find("cmd=0104") == 0) {
                rec = "cmd=0104,ret=0,data=null" + s;
                cout << "UDPClient rec(fake) " << rec << endl;
            } else {
                cout << "UDPClient" << endl;
                rec = UDPClient(recvBuf) + s;
                cout << "UDPClient rec " << rec << endl;
            }
        }
        ti();
        strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
        sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server, len_server);
    }

}

string my::UDPClient(char *sendBuf) {
    //加载套接字库
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    char recvBuf[1000] = {0};    //收

    wVersionRequested = MAKEWORD(1, 1);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        return "";
    }

    if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
        HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
    {
        WSACleanup();
        return "";
    }

    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addrSrv;
    addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(10001);

    int len = sizeof(SOCKADDR);
    sendto(sockSrv, sendBuf, strlen(sendBuf) + 1, 0, (SOCKADDR *) &addrSrv, len);
    //等待并数据
    recvfrom(sockSrv, recvBuf, 1000, 0, (SOCKADDR *) &addrSrv, &len);

    if ('q' == recvBuf[0]) {
        sendto(sockSrv, "q", strlen("q") + 1, 0, (SOCKADDR *) &addrSrv, len);
        printf("Chat end!\n");
    }
    string s = recvBuf;
    //发送数据
    return s;

}

void my::FuckTheCJ(string rec) {
    strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
    for (int i = 0; i < 5; ++i) {
        Sleep(1000);
        sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
               len_server);
        cout << "serial rec:" << rec << endl;
    }
}

int main() {
    my *a = new my();


}

//cmd=1001,ret=0,data=1;682200C30506060200010000CD29850100F10002000109074C5931303236360000CE0616");

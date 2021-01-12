
#include "myH.h"
#include <iostream>
#include <thread>
#include <regex>
#include <zconf.h>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

void StringToHex(string str, unsigned char *buff) {
    int j = 0;
    int z = 0;
    for (int i = 0; i < (str.length() / 2); i++) {
        string temp = str.substr(z, 2);
//        cout<<"temp:"<<temp<<endl;
        buff[j++] = stoi(temp, 0, 16);
        z += 2;
    }
}

void ti() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    cout << ltm->tm_hour << ":";
    cout << ltm->tm_min << ":";
    cout << ltm->tm_sec << "\n------------------------------------------\n";
}

vector<MultiSerial *> serial_prt;
vector<int> serial_map;

MultiSerial::MultiSerial(const std::string &num, const std::string &com) : MultiNum(num) {
    string temp = "\\\\.\\COM" + com;
    LPCSTR s = temp.data();
    MultihCom = CreateFile(s,
                           GENERIC_READ | GENERIC_WRITE, //允许读和写
                           0, //独占方式
                           NULL,
                           OPEN_EXISTING, //打开而不是创建
                           0, //同步方式
                           NULL);
    if (MultihCom == (HANDLE) -1) {
        cout << com << " open fail!\n";
        return;
    }
    SetupComm(MultihCom, 2048, 2048); //输入缓冲区和输出缓冲区的大小都是1024
    COMMTIMEOUTS TimeOuts; //设定读超时
    TimeOuts.ReadIntervalTimeout = MAXDWORD;
    TimeOuts.ReadTotalTimeoutMultiplier = 0;
    TimeOuts.ReadTotalTimeoutConstant = 0;
    TimeOuts.WriteTotalTimeoutMultiplier = 500;
    TimeOuts.WriteTotalTimeoutConstant = 500;
    SetCommTimeouts(MultihCom, &TimeOuts);
    DCB dcb;
    GetCommState(MultihCom, &dcb);
    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity = EVENPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(MultihCom, &dcb);
    PurgeComm(MultihCom, PURGE_TXCLEAR | PURGE_RXCLEAR);
    cout << "COM" << com << " Prepared..." << endl;
}

void MultiSerial::MultiRead(MultiSerial *ptr) {
    if (ptr == (HANDLE) -1) {
        return;
    }
    char str[2000];
    DWORD wCount;//读取的字节数
    BOOL bReadStat;
    string output;
    vector<string> map;
    char a = 0x0d;
    char b = 0x0a;
    string s;
    s.push_back(a);
    s.push_back(b);
    while (true) {
        sleep(1);
        bReadStat = ReadFile(ptr->MultihCom, str, 2000, &wCount, NULL);
        if (!bReadStat) {
            cout << "Pos: " << ptr->MultiNum << " read failed!\n";
            return;
        }
        if (wCount > 0) {
            char temp[4] = {0};
            for (int i = 0; i < wCount; i++) {
                sprintf(temp, "%02X", (BYTE) str[i]);
                map.push_back(temp);
            }
            cur:
            if (map.size() >= 15) {
                if (map[0] == "68") {
                    //698
                    if (map[14] == "85" and map[15] == "01" and map[1] == "22" and map[4] == "05") {
                        if (36 > map.size())
                            continue;
                        for (int i = 0; i < 36; ++i) {
                            output += map[i];
                        }
                        string rec = "cmd=1001,ret=0,data=" + ptr->MultiNum + ";" + output + s;
                        for (int i = 0; i < 5; ++i) {
                            mtx_SerialToUDPServer.lock();
                            SerialToUDPServer.emplace_back(rec);
                            mtx_SerialToUDPServer.unlock();
                        }
                        cout << "SerialToUDPServer: " << rec;
                        ti();
                        map.erase(begin(map), begin(map) + 24);
                        output.clear();
                        goto cur;
                    }
                        //376.2
                    else {
                        cout << "376.2\n";
                        int len = (stoi(map[1], 0, 16)) + (stoi(map[2], 0, 16) << 8);
                        if (len > 260) {
                            cout << "len to long !! erase:" << map[0] << endl;
//                            write("erase:" + map[0]);
                            map.erase(begin(map));
                            goto cur;
                        }
                        if (len > map.size())
                            continue;
                        for (int i = 0; i < len; ++i) {
                            output += map[i];
                        }
                        string rec = "cmd=1001,ret=0,data=" + ptr->MultiNum + ";" + output + s;
                        mtx_SerialToUDPServer.lock();
                        SerialToUDPServer.emplace_back(rec);
                        mtx_SerialToUDPServer.unlock();
                        cout << "SerialToUDPServer: " << rec;
                        ti();
                        map.erase(begin(map), begin(map) + len);
                        output.clear();
                        goto cur;
                    }
                } else {
                    while (map.size() > 15 && map[0] != "68") {
                        cout << "erase:" << map[0] << endl;
                        map.erase(begin(map));
                    }
                    if (map[0] == "68")
                        goto cur;
                }
            }
        }
    }
}

void MultiSerial::MuliWrite(std::string text) {
    BYTE lpOutBuffer[1500] = {0};
    nApduLen = text.length() / 2;
    StringToHex(text, lpOutBuffer);
    COMSTAT ComStat;
    DWORD dwErrorFlags;
    BOOL bWriteStat;
    ClearCommError(MultihCom, &dwErrorFlags, &ComStat);
    bWriteStat = WriteFile(MultihCom, lpOutBuffer, nApduLen, &nApduLen, NULL);
    if (!bWriteStat) { cout << "写串口失败!"; }
    lpOutBuffer[0] = {0};
}

UDP_Server::UDP_Server() {
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
    //创建用于监听的套接字
    sockSrv_server = socket(AF_INET, SOCK_DGRAM, 0);//失败会返回 INVALID_SOCKET
    SOCKADDR_IN addrSrv;     //定义sockSrv发送和接收数据包的地址
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(10002);
    bind(sockSrv_server, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));//会返回一个SOCKET_ERROR
    cout << "UDP Server is ready\n";
    len_server = sizeof(SOCKADDR);
    char a = 0x0d;
    char b = 0x0a;
    string s;
    s.push_back(a);
    s.push_back(b);
    string rec;
    thread t(&UDP_Server::CheckFromToUDPServer, this);
    t.detach();
    while (true) {
        sleep(1);
        char recvBuf[2000] = {0};
        recvfrom(sockSrv_server, recvBuf, 2000, 0, (SOCKADDR *) &addrClient_server, &len_server);
        if (recvBuf[0] == 0)
            continue;
        string recvBuf_s = recvBuf;
        regex e("cmd=1001,data=.+;0;");
        smatch m;
        bool found = regex_search(recvBuf_s, m, e);
        if (found) {
            string hex_message = m.suffix().str();
            regex e2("data=[0-9]{1,2}");
            smatch m2;
            regex_search(recvBuf_s, m2, e2);
            string MeterPosition = m2.str(0).substr(5);
//            cout<<"MeterPosition:"<<MeterPosition<<endl;
//            cout<<"MuliWrite:"<<m2.suffix().str().substr(3)<<endl;
            int n = atoi(MeterPosition.c_str());
            serial_prt[n - 1]->MuliWrite(m2.suffix().str().substr(3));

        } else {
            if (recvBuf_s.find("cmd=0104") == 0) {
                sleep(1);
                cout << "\n-------------UDPClient--------------" << endl;
                rec = "cmd=0104,ret=0,data=null" + s;
                cout << "UDPClient rec(fake) " << rec << endl;
                strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
                sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
                       len_server);
            } else {
                cout << "\n-------------UDPClient--------------" << endl;//to UDP Client
                mtx_UDPServerToClient.lock();
                UDPServerToClient.emplace_back(recvBuf_s);
                mtx_UDPServerToClient.unlock();
            }
        }

    }
}

void UDP_Server::CheckFromToUDPServer() {
    while (true) {
        sleep(1);
        mtx_UDPClientToServer.lock();
        while (!UDPClientToServer.empty()) {
            string rec = UDPClientToServer[0];
            cout << "UDPClientToServer " << UDPClientToServer[0] << endl;
            strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
            sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
                   len_server);
            UDPClientToServer.erase(UDPClientToServer.begin(), UDPClientToServer.begin() + 1);
        }
        mtx_UDPClientToServer.unlock();
        mtx_SerialToUDPServer.lock();
        while (!SerialToUDPServer.empty()) {
            string rec = SerialToUDPServer[0];
            cout << "SerialToUDPServer " << SerialToUDPServer[0] << endl;
            strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
            sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
                   len_server);
            SerialToUDPServer.erase(SerialToUDPServer.begin(), SerialToUDPServer.begin() + 1);
        }
        mtx_SerialToUDPServer.unlock();

    }
}

UDP_Client::UDP_Client() {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    char recvBuf[1000] = {0};
    wVersionRequested = MAKEWORD(1, 1);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        cout << "ERROR 0\n";
        return;
    }
    if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
        HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
    {
        cout << "ERROR 1\n";
        WSACleanup();
        return;
    }
    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addrSrv;
    addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(10001);

    struct timeval timeout = {3, 0};
    setsockopt(sockSrv, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
    cout << "UDP Client is ready\n";
    int len = sizeof(SOCKADDR);
    //等待并数据
    while (true) {
        sleep(1);
        mtx_SerialToUDPServer.lock();
        while (!UDPServerToClient.empty()) {
            string rec = UDPServerToClient[0];
            cout << "SerialToUDPServer " << UDPServerToClient[0] << endl;
            sendto(sockSrv, rec.c_str(), strlen(rec.c_str()) + 1, 0,
                   (SOCKADDR *) &addrSrv, len);
            UDPServerToClient.erase(UDPServerToClient.begin(), UDPServerToClient.begin() + 1);
        }
        mtx_SerialToUDPServer.unlock();

        recvfrom(sockSrv, recvBuf, 1000, 0, (SOCKADDR *) &addrSrv, &len);
        if (recvBuf[0] == 0) {
            continue;
        }
        string s = recvBuf;
        mtx_UDPClientToServer.lock();
        UDPClientToServer.emplace_back(s);
        mtx_UDPClientToServer.unlock();
        recvBuf[0] = {0};

    }
}

void ThreadUDPS() {
    auto thread_s = new UDP_Server();

}

void ThreadUDPC() {
    auto thread_c = new UDP_Client();

}

int main() {
    thread t1(ThreadUDPC);

    t1.detach();


    map<string, string> serial_list;
    serial_list.insert(pair<string, string>("1", "34"));
    serial_list.insert(pair<string, string>("2", "19"));
    serial_list.insert(pair<string, string>("3", "20"));
    serial_list.insert(pair<string, string>("4", "21"));
    serial_list.insert(pair<string, string>("5", "22"));
    serial_list.insert(pair<string, string>("6", "23"));
    serial_list.insert(pair<string, string>("7", "24"));
    serial_list.insert(pair<string, string>("8", "25"));
    serial_list.insert(pair<string, string>("9", "26"));
    serial_list.insert(pair<string, string>("10", "27"));
    serial_list.insert(pair<string, string>("11", "28"));
    serial_list.insert(pair<string, string>("12", "29"));
    serial_list.insert(pair<string, string>("13", "30"));
    serial_list.insert(pair<string, string>("14", "31"));
    serial_list.insert(pair<string, string>("15", "32"));
    serial_list.insert(pair<string, string>("16", "33"));
    for (int i = 1; i <= serial_list.size(); ++i) {
        auto start = new MultiSerial(to_string(i), serial_list.at(to_string(i)));
        serial_prt.emplace_back(start);
        serial_map.emplace_back(i);
        std::thread t(start->MultiRead, start);
        t.detach();
//        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    cout << "Done\n";
    thread t2(ThreadUDPS);
    t2.join();

}



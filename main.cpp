#include "myH.h"
#include <iostream>
#include <thread>
#include <regex>
#include <zconf.h>

using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#define SLOW 1

enum Mstyle {
    P645 = 0,
    P376_1,
    P698,
    PNone,
    P376_2,
};

vector<Mstyle> flagsSum = {Mstyle::P698, Mstyle::P698, Mstyle::P698, Mstyle::P698, Mstyle::P698, Mstyle::P698,
                           Mstyle::P698, Mstyle::P698, Mstyle::P698, Mstyle::P698, Mstyle::P698, Mstyle::P698,
                           Mstyle::P698, Mstyle::P698, Mstyle::P698, Mstyle::P698};

inline Mstyle checkStat(MultiSerial *prt) {
    std::string str = prt->MultiNum;
    return flagsSum[atoi(str.c_str()) - 1];
}

inline void setStat(int n, Mstyle s) {
    flagsSum[n] = s;
}

void StringToHex(string str, unsigned char *buff) {
    int j = 0;
    int z = 0;
    for (int i = 0; i < (str.length() / 2); i++) {
        string temp = str.substr(z, 2);
        buff[j++] = stoi(temp, nullptr, 16);
        z += 2;
    }
}

void mySleep(int msec) {
    clock_t now = clock();
    while (clock() - now < msec);
}

void write(string temp) {
    ofstream outfile(R"(out.txt)", ios::app);
    outfile << temp;
    time_t now = time(0);
    tm *ltm = localtime(&now);
    outfile << endl;
    outfile << ltm->tm_hour << ":";
    outfile << ltm->tm_min << ":";
    outfile << ltm->tm_sec << "\n";
    outfile << endl;
    outfile.close();
}

void ti() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    cout << ltm->tm_hour << ":";
    cout << ltm->tm_min << ":";
    cout << ltm->tm_sec << "\n------------------------------------------\n";
}

WSAEVENT g_hServerEvent;                                         //server 网络事件对象
SOCKET g_sClient[WSA_MAXIMUM_WAIT_EVENTS] = {INVALID_SOCKET};  //client socket数组
WSAEVENT g_event[WSA_MAXIMUM_WAIT_EVENTS];                     //网络事件对象数组

vector<MultiSerial *> serial_prt;

MultiSerial::MultiSerial(const std::string &num, const std::string &com) : MultiNum(num) {
    string temp = "\\\\.\\COM" + com;
    LPCSTR s = temp.data();
    MultihCom = CreateFile(s,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);
    if (MultihCom == (HANDLE) -1) {
        cout << com << " open fail!\n";
        return;
    }
    SetupComm(MultihCom, 2048, 2048);
    COMMTIMEOUTS TimeOuts;
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
    DWORD wCount;
    BOOL bReadStat;
    string output;
    vector<string> map;
    char a = 0x0d;
    char b = 0x0a;
    string s;
    s.push_back(a);
    s.push_back(b);
    while (true) {
        Sleep(80);
        bReadStat = ReadFile(ptr->MultihCom, str, 2000, &wCount, NULL);
        if (!bReadStat) {
            cout << "Pos: " << ptr->MultiNum << " read failed!\n";
            return;
        }
        if (wCount > 0) {
            char temp[4] = {0};
            for (int i = 0; i < wCount; i++) {
                sprintf(temp, "%02X", (BYTE) str[i]);
                map.emplace_back(temp);
            }
            cur:
            if (map.size() >= 15) {
                if (map[0] == "68") {
                    //698
                    if (map[14] == "85" and map[1] == "22" and map[4] == "05") {
                        if (36 > map.size())
                            continue;
                        for (int i = 0; i < 36; ++i) {
                            output += map[i];
                        }
                        int len698 = 2 + (stoi(map[1], nullptr, 16)) + (stoi(map[2], nullptr, 16) << 8);

                        if (checkStat(ptr) == Mstyle::P698) {
                            string rec = "cmd=1001,ret=0,data=" + ptr->MultiNum + ";" + output + s;
                            mtx_SerialToUDPServer.lock();
                            SerialToUDPServer.emplace_back(rec);
                            mtx_SerialToUDPServer.unlock();
                        }
                        ti();
                        map.erase(begin(map), begin(map) + len698);
                        output.clear();
                        goto cur;
                    }
                        //376.2
                    else {
                        if (map[2] != "00") {
                            cout << "map[2] != 00 chang it!" << endl;
                            map[2] = "00";
                            string err;
                            for (auto &i : map) {
                                err += i + " ";
                            }
                            cout << "err: " << err << endl;
                            write("err: " + err);
                            goto cur;
                        }
                        int len = (stoi(map[1], nullptr, 16)) + (stoi(map[2], nullptr, 16) << 8);
                        if (len > map.size())
                            continue;
                        if (len > 250) {
                            for (int i = 0; i < 120; ++i) {
                                output += map[i] + " ";
                            }
                            cout << "wrong message: " << output << endl;
                            output.clear();
                            cout << "len to long !! erase:" << map[0] << endl;
//                            write("len to long !! erase: " + map[0]);
                            map.erase(begin(map));
                            goto cur;
                        }
                        if (map[len - 1] != "16") {
                            cout << "Detective message error! not end with 16\n";
//                            write("not end with 16");
                            string save;
                            for (auto &i : map) {
                                save += i;
                            }
//                            write(save);
//                            write("----save----");

                            for (int i = 0; i < len; ++i) {
//                                write(*(begin(map)));
                                cout << "erase:" << map[0] << endl;
                                map.erase(begin(map));
                            }
//                            write("----erase----");
                            goto cur;
                        }
                        for (int i = 0; i < len; ++i) {
                            output += map[i];
                        }
                        if (checkStat(ptr) == Mstyle::P376_2) {
                            string rec = "cmd=1001,ret=0,data=" + ptr->MultiNum + ";" + output + s;
                            mtx_SerialToUDPServer.lock();
                            SerialToUDPServer.emplace_back(rec);
                            mtx_SerialToUDPServer.unlock();
                            write(output);
//                            write("-----");
                            map.erase(begin(map), begin(map) + len);
                        } else {
                            for (int i = 0; i < len; ++i) {
//                                write(*(begin(map)));
                                map.erase(begin(map));
                            }
//                            write("----erase above----");
                        }
                        output.clear();
                        goto cur;
                    }
                } else {
                    while (map[0] != "68") {
                        if (map.size()==1){
                            break;
                        }
                        cout << "erase:" << map[0] << endl;
//                        write(*(begin(map)));
                        map.erase(begin(map));
                    }
//                    write("----------");
                    if (map[0] == "68")
                        goto cur;
                    else
                        continue;
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
    write(text);
    if (!bWriteStat) { cout << "写串口失败!"; }
    lpOutBuffer[0] = {0};
}

void MultiSerial::WriteWhile(MultiSerial *ptr) {
    while (true){
        Sleep(1000);
        if (!ptr->WriteQue.empty()){
            string mess = ptr->WriteQue[0];
            ptr->MuliWrite(mess);
            ptr->WriteQue.erase(ptr->WriteQue.begin());
        }
    }
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

    //------------------------注册------------------------
    g_hServerEvent = WSACreateEvent();                    //创建网络事件对象
    WSAEventSelect(sockSrv_server, g_hServerEvent, FD_ACCEPT);//为server socket注册网络事件
    WSANETWORKEVENTS networkEvents; //网络事件结构


    SOCKADDR_IN addrSrv;     //定义sockSrv发送和接收数据包的地址
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(10002);
    bind(sockSrv_server, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));//会返回一个SOCKET_ERROR
    cout << "UDP Server is ready...\n";
    len_server = sizeof(SOCKADDR);
    char a = 0x0d;
    char b = 0x0a;
    string s;
    s.push_back(a);
    s.push_back(b);
    string rec;
    thread t(&UDP_Server::CheckFromToUDPServer, this);
    thread t2(&UDP_Server::CheckFromUDPClient, this);
    t.detach();
    t2.detach();
    while (true) {
        Sleep(80);
        char recvBuf[1024] = {0};
        recvfrom(sockSrv_server, recvBuf, 1024, 0, (SOCKADDR *) &addrClient_server, &len_server);
        if (recvBuf[0] == 0) {
            continue;
        }
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
            int n = atoi(MeterPosition.c_str());
            serial_prt[n-1]->WriteQue.emplace_back(m2.suffix().str().substr(3));
//            serial_prt[n - 1]->MuliWrite(m2.suffix().str().substr(3));
            cout << "\n-------------UDPClient--------------" << endl;
            cout << "ServerToSerial :" << recvBuf_s << endl;

        } else {
            if (recvBuf_s.find("cmd=0104,data=0") == 0) {
                cout << "\n-------------UDPClient--------------" << endl;
                cout << "From CJ rec:" << recvBuf_s << endl;
                char Mstyle_ = recvBuf_s[16];
                string s1;
                s1.push_back(Mstyle_);
                int n = atoi(s1.c_str());
                char Mpos = recvBuf_s[18];
                string s2;
                s2.push_back(Mpos);
                int n2 = atoi(s2.c_str()) - 1;
                switch (n) {
                    case 0: {
                        setStat(n2, Mstyle::P645);
                        cout << "P645\n";
                    }
                        break;
                    case 1: {
                        setStat(n2, Mstyle::P376_1);
                        cout << "P376\n";
                    }
                        break;
                    case 2: {
                        setStat(n2, Mstyle::P698);
                        cout << "P698\n";
                    }
                        break;
                    case 4: {
                        setStat(n2, Mstyle::P376_2);
                        cout << "P376_2\n";
                    }
                        break;
                    default:
                        cout << "Invalid!!!";
                }
                rec = "cmd=0104,ret=0,data=null" + s;
                cout << "UDPClient rec(fake): " << rec << endl;
                strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
                sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
                       len_server);
            } else {
                cout << "\n-------------to UDP Client--------------" << endl;//to UDP Client
                mtx_UDPServerToClient.lock();
                UDPServerToClient.emplace_back(recvBuf_s);
                mtx_UDPServerToClient.unlock();
            }
        }

    }
}

void UDP_Server::CheckFromUDPClient() {
    while (true) {
        Sleep(80);
        while (!UDPClientToServer.empty()) {
            mtx_UDPClientToServer.lock();
            string rec = UDPClientToServer[0];
            cout << "UDPClientToServer: " << UDPClientToServer[0] << endl;
            strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
            sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
                   len_server);
            UDPClientToServer.erase(UDPClientToServer.begin());
            ti();
            mtx_UDPClientToServer.unlock();
        }
    }
}

void UDP_Server::CheckFromToUDPServer() {
    while (true) {
        Sleep(80);
        while (!SerialToUDPServer.empty()) {
            mtx_SerialToUDPServer.lock();
            string rec = SerialToUDPServer[0];
            cout << "SerialToUDPServer: " << SerialToUDPServer[0] << endl;
            strncpy(sendBuf_server, rec.c_str(), rec.length() + 1);
            sendto(sockSrv_server, sendBuf_server, strlen(sendBuf_server), 0, (SOCKADDR *) &addrClient_server,
                   len_server);
            SerialToUDPServer.erase(SerialToUDPServer.begin());
            ti();
            mtx_SerialToUDPServer.unlock();
        }
    }
}

UDP_Client::UDP_Client() {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

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

    struct timeval timeout = {2, 0};
    setsockopt(sockSrv, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
    cout << "UDP Client is ready...\n";
    int len = sizeof(SOCKADDR);
    //等待并数据
    std::thread t_rec(&UDP_Client::rec, this, sockSrv, addrSrv);
    t_rec.detach();
    while (true) {
        Sleep(80);
        while (!UDPServerToClient.empty()) {
            mtx_SerialToUDPServer.lock();
            string rec = UDPServerToClient[0];
            cout << "UDPServerToClient: " << UDPServerToClient[0] << endl;
            sendto(sockSrv, rec.c_str(), strlen(rec.c_str()), 0,
                   (SOCKADDR *) &addrSrv, len);
            UDPServerToClient.erase(UDPServerToClient.begin());
            mtx_SerialToUDPServer.unlock();
        }
    }
}

void UDP_Client::rec(SOCKET sockSrv, sockaddr_in addrSrv) {
    while (true) {
        Sleep(80);
        int len = sizeof(SOCKADDR);
        char recvBuf[1000] = {0};
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
        std::thread t(start->MultiRead, start);
        t.detach();
        std::thread t2(start->WriteWhile,start);
        t2.detach();

    }
    cout << "All serial prepared...\n";
    thread t2(ThreadUDPS);
    t2.join();
}



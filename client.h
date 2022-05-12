#ifndef CLIENT_H
#define CLIENT_H

#include "global.h"
//const int MAX_EVENT_NUMBER = 10000;
//封装客户端的类
class my_client{

public:
    my_client(std::string ip = "127.0.0.1", int port = 9999);
    ~my_client();
    void run();
    static void SendMsg(int connfd);//发送线程
    static void RecvMsg(int connfd);//接收线程

    //登录和注册
    void HandleClient(int connfd);

private:
    //服务器的IP地址
    int my_server_port;
    //服务器的端口
    std::string my_server_ip;
    //建立连接的套接字文件描述符
    int my_clientfd;

};
#endif
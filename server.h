#ifndef SERVER_H
#define SERVER_H
#include "global.h"
//const int MAX_EVENT_NUMBER = 10000;

//进行一个迭的代
//接下来是多线程的服务器
class my_server{

public:
    my_server(std::string my_server = "127.0.0.1", int port = 9999);
    ~my_server();

    void run();//服务器开始运行
    static void work(int epollfd, int connfd);

    static void HandleRequest(int epollfd, int connfd, std::string str,
                    std::tuple<bool, std::string, std::string, int, int> &info);

    static void setnonblocking(int connfd); //设置套接字为非阻塞

private:
    //服务器IP地址
    std::string my_server_ip;
    
    //服务器端口号
    int my_server_port;
    
    //监听的套接字文件描述符
    int my_listenfd;
    //保存客户端套接字描述符的容器
    //std::vector<int> my_connfds;
    static std::vector<bool> my_connfds;//改为了静态成员变量，且类型变为vector<bool>

    //名字和套接字描述符
    static std::unordered_map<std::string, int> name_sock_map;

    //锁
    static pthread_mutex_t name_sock_mutex;//保护name_sock_map

    //记录群号和套接字集合
    static std::unordered_map<int, std::set<int>> group_map;

    //锁 保护group_map
    static pthread_mutex_t group_mutex;

    static std::unordered_map<std::string, std::string> from_to_map;//记录用户向用户私聊的名单

    //锁 form_to_map
    static pthread_mutex_t from_to_mutex;


};
#endif
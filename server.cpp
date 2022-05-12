
#include "server.h"
std::vector<bool> my_server::my_connfds(10240,false);    //将10000个位置都设为false，sock_arr[i]=false表示套接字描述符i未打开（因此不能关闭）
//记录登录状态
std::unordered_map<std::string, int> my_server::name_sock_map;
pthread_mutex_t my_server::name_sock_mutex;
//群聊
std::unordered_map<int, std::set<int>> my_server::group_map;
pthread_mutex_t my_server::group_mutex;
//私聊
std::unordered_map<std::string, std::string> my_server::from_to_map;//记录用户向用户私聊的名单
pthread_mutex_t my_server::from_to_mutex;



my_server::my_server(std::string ip, int port) : my_server_ip(ip), my_server_port(port)
{
    //锁的初始化
    pthread_mutex_init(&name_sock_mutex, 0);

    pthread_mutex_init(&group_mutex, 0);

    pthread_mutex_init(&from_to_mutex, 0);
    //随机种子
    srand((unsigned) time(NULL));
}

my_server::~my_server()
{
    for(int i=0;i<my_connfds.size();i++){
        if(my_connfds[i])
            close(i);
    }
    close(my_listenfd);
    pthread_mutex_destroy(&name_sock_mutex);
    pthread_mutex_destroy(&group_mutex);
    pthread_mutex_destroy(&from_to_mutex);
}

void my_server::setnonblocking(int connfd)
{
    int flags = fcntl(connfd, F_GETFL);
    flags = flags | O_NONBLOCK;
    fcntl(connfd, F_SETFL, flags);
}



//进行一个迭的代
//首先来个简单的服务器和客户端
void my_server::run()
{
    
    //socket
    my_listenfd = socket(AF_INET, SOCK_STREAM, 0);

    //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件
    struct epoll_event ev, events[MAX_EVENT_NUMBER];
    //创建epoll树
    int epfd = epoll_create(MAX_EVENT_NUMBER);

    //把my_listenfd设置为非阻塞
    setnonblocking(my_listenfd);

    ev.data.fd = my_listenfd;
    ev.events = EPOLLIN | EPOLLRDHUP;

    epoll_ctl(epfd, EPOLL_CTL_ADD, my_listenfd, &ev);

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(my_server_port);
    inet_pton(AF_INET, my_server_ip.c_str(), &address.sin_addr.s_addr );

    int ret =  bind(my_listenfd, (struct sockaddr *) &address, sizeof(address));
    assert(ret != -1);
    
    ret = listen(my_listenfd, 200);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addrlen = sizeof(client_addr);

    int maxi = 0;

    //定义一个8线程的线程池
    boost::asio::thread_pool tp(8);
    while(1)
    {
        std::cout<<"--------------------------"<<std::endl;
        std::cout<<"epoll_wait阻塞中"<<std::endl;
        //等待epoll事件的发生
        int nfds = epoll_wait(epfd, events, 10000, -1);//最后一个参数是timeout，0:立即返回，-1:一直阻塞直到有事件，x:等待x毫秒
        std::cout << "epoll_wait返回,有事件发生" << std::endl;
        //处理所发生的所有事件
        for(int i = 0; i < nfds; i++)
        {
            //如果是新客户端连接服务器
            if(events[i].data.fd == my_listenfd)
            {
                int connfd = accept(my_listenfd, (sockaddr*)&client_addr, &client_addrlen);
                if(connfd < 0)
                {
                    perror("connfd < 0");
                    exit(1);
                }
                else{
                    std::cout << "用户" << inet_ntoa(client_addr.sin_addr) << "正在连接\n";
                }
                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLONESHOT;

                setnonblocking(connfd);
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

            }
            //接收到读事件
            else if(events[i].events & EPOLLIN)
            {
                int sockfd = events[i].data.fd;
                events[i].data.fd = -1;
                std::cout << "接收到读事件" << std::endl;

                std::string recv_str;

                //加入任务队列，处理事件
                boost::asio::post(boost::bind(work, epfd, sockfd));
            }
        }
        
        // int connfd = accept(my_listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
        // if(connfd < 0)
        // {
            
        //      perror("connect");
        //     exit(1);
        // }
        // my_connfds[connfd] = true;
        // std::thread t(work, connfd);
        // t.detach();//线程分离
    }
    
}
/*
接下来修改服务器端的代码，服务器端需要修改的地方较多。
在之前的服务器代码中，我们利用 RecvMsg 线程函数不断接收信息，
然后在该函数中调用 HandleRequest 来处理具体的逻辑业务，
但是我们要想记住当前服务对象的一些状态信息只能在 RecvMsg 函数中保存，
而我们希望在 HandleRequest 对这些状态做出的更新能同步到 RecvMsg 函数中，
因此需要使用引用传参方式。

因此我们修改 RecvMsg 和 HandleRequest，用一个 tuple 
元组引用类型（相当于一个结构体）作为参数传入 HandleRequest，
元组中全是我们需要更新同步的状态信息。

*/

void my_server::work(int epollfd, int connfd){
    std::tuple<bool, std::string, std::string, int, int> info;
    //元组类型，五个成员分别为login_state、login_name、target_name、target_connfd, group_num

    // bool login_state = false; //记录当前服务对象是否成功登录
    // std::string login_name;//记录当前服务对象的名字
    // std::string target_name;//记录发送信息时目标用户的名字
    // int target_connfd; //目标对象的套接字描述符
    // int group_num; //记录群号
    std::get<0>(info) = false; //初始化login_state为false
    std::get<3>(info) = -1;    //初始化target_connfd为-1
    std::get<4>(info) = -1;    //初始化群号为-1
    char buffer[MAX_EVENT_NUMBER];
    std::string recv_str;
    while(1)
    {
        memset(&buffer, 0, sizeof(buffer));
        int ret = recv(connfd, &buffer, sizeof(buffer), 0);
        if(ret < 0)
        {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                std::cout<<"数据读取完毕\n";
                std::cout<<"接收到的完整内容为："<<recv_str<< std::endl;
                std::cout<<"开始处理事件"<<std::endl;
                break;
            }
            std::cout << "errno:"<< errno << std::endl;
            my_connfds[connfd] = false;
            close(connfd);
            return;
        }
        else if(ret == 0 || strcmp(buffer, "exit") == 0 )
        {
            std::cout<<"recv返回值为0"<< std::endl;
            std::cout << "客户端断开连接" << std::endl;
            my_connfds[connfd] = false;
            close(connfd);
            return;
        }
        else 
        {
            std::cout <<"收到套接字描述符为"<< connfd << "的客户端发送:" << buffer << std::endl; 
            std::string str(buffer);
            std::cout << str << std::endl;
            recv_str += str;
        }
        
    }    
    
    HandleRequest(epollfd, connfd, recv_str, info);
}

//连接数据库  
//并且对str进行分析解析  name  和  pass 
//到数据库进行注册
void my_server::HandleRequest(int epollfd, int connfd, std::string str, std::tuple<bool, std::string, std::string, int, int> &info)
{
    
    char buffer[1000];
    std::string name, pass;
    //把参数提出来，方便操作
    bool login_state = std::get<0>(info); //记录当前服务对象是否成功登录
    std::string login_name = std::get<1>(info);//记录当前服务对象的名字
    std::string target_name = std::get<2>(info);//记录发送信息时目标用户的名字
    int target_connfd = std::get<3>(info); //记录目标用户的套接字
    int group_num = std::get<4>(info); //记录所处群号

    //int group_num;  //记录群号


    //连接MYSQL数据库
    //初始化数据库类并连接到本地数据库
    MYSQL *con = NULL;
      
    con = mysql_init(con);//放入本地数据库
                                //也就是初始化一个连接句柄

    //如果连接成功，一个 MYSQL*连接句柄。如果连接失败，NULL。
    //对一个成功的连接，返回值与第一个参数值相同，除非你传递NULL给该参数。
    //con = mysql_real_connect(con, "127.0.0.1", "root", "515711", "ChatProject", 3306, NULL, CLIENT_MULTI_STATEMENTS);
    //std::cout << con<< std::endl;
    //数据库连接失败，错误处理
    if(!mysql_real_connect(con, "127.0.0.1", "root", "515711", "ChatProject", 3306, NULL, CLIENT_MULTI_STATEMENTS)){
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(EXIT_FAILURE);
    }

    //连接Redis数据库
    redisContext *redis_target = redisConnect("127.0.0.1", 6379);
    if(redis_target -> err)
    {
        redisFree(redis_target);
        std::cout << "连接redis失败" << std::endl;
    }


    //通过redis来实现seesion的功能
    //先接收cookie看看redis是否保存该用户的登录状态
    //当用户登录时 会使用cookie记录登录信息
    //下次登录时就会直接发送cookie来登录
    //如果redis中的键过期了 则需要重新登录
    if(str.find("cookie:") != str.npos)
    {
        std::cout<<"cookie方法\n";
        int p1 = str.find("cookie:");
        int key1_len=strlen("cookie:");
        std::string cookie = str.substr(p1 + key1_len, str.length() - key1_len);

        //查询redis cookie是否存在
        std::string redis_str = "hget " + cookie + " name";

        redisReply *r = (redisReply*)redisCommand(redis_target, redis_str.c_str());

        std::string send_seesion_res;
        //查询不为空
        if(r -> str)
        {
            std::cout << "查询redis结果:" << r->str << std::endl;
            send_seesion_res = r -> str;

            //刷新生存时间
            std::string ttl_str = "expire " + cookie + " 300";

            r = (redisReply*)redisCommand(redis_target, ttl_str.c_str());
        }
        else 
            send_seesion_res = "NULL";

        send(connfd, send_seesion_res.c_str(), sizeof(send_seesion_res), 0);
    }


    if(str.find("register:") != str.npos)
    {
        std::cout<<"注册方法\n";
        int p1 = str.find("name:");
        int p2 = str.find("pass:");
        int key1_len=strlen("name:"),key2_len=strlen("pass:");
        name = str.substr(p1 + key1_len, p2 - p1 - key1_len);
        pass = str.substr(p2 + key2_len, str.length() - p2 - key2_len);

        std:: string search="INSERT INTO USER VALUES (\"";
        search+=name;
        search+="\",\"";
        search+=pass;
        search+="\");";
        std::cout << "sql语句:" << search << std::endl;
        if(mysql_query(con,search.c_str())){
            fprintf(stderr, "%s\n", mysql_error(con));
            mysql_close(con);
            exit(EXIT_FAILURE);
        }
        //mysql_query(con, "SELECT * FROM USER");
        std::cout << "sql语句:" << search << std::endl;
        
    }
    //登录
    else if(str.find("login:") != str.npos)
    {
        std::cout<<"登录方法\n";
        int p1 = str.find("name:");
        int p2 = str.find("pass:");
        int key1_len=strlen("name:"),key2_len=strlen("pass:");
        name = str.substr(p1 + key1_len, p2 - p1 - key1_len);
        pass = str.substr(p2 + key2_len, str.length() - p2 - key2_len);

        std::string search = "SELECT * FROM USER WHERE NAME=\"";
        search += name;
        search += "\";";
        std::cout << "sql语句:" << search << std::endl;

        auto search_res = mysql_query(con, search.c_str());
        auto result = mysql_store_result(con);
        int col = mysql_num_fields(result); //获取列数
        int row = mysql_num_rows(result); //获取行数

        //查询到用户名
        if(search_res == 0 && row != 0)
        {
            std::cout << "查询成功\n";
            auto info = mysql_fetch_row(result);//获取一行的信息
            
            std::cout << "查询到用户名:" << info[0] <<" 密码:" << info[1] << std::endl;

            if(info[1] == pass){
                std::cout << "登录密码正确\n\n";
                std::string str1 = "OK";
                login_state = true;
                login_name = name;

                //上锁
                pthread_mutex_lock(&name_sock_mutex);

                name_sock_map[login_name] = connfd;
                //记录下名字和文件描述符的对应关系

                //解锁
                pthread_mutex_unlock(&name_sock_mutex);

                // 随机生成sessionid并发送到客户端
                for (int i = 0; i < 10; i++)
                {
                    int type = rand() % 3; // type为0代表数字，为1代表小写字母，为2代表大写字母
                    if (type == 0)
                        str1 += '0' + rand() % 9;
                    else if (type == 1)
                        str1 += 'a' + rand() % 26;
                    else if (type == 2)
                        str1 += 'A' + rand() % 26;
                }
                //将session保存到redis中
                std::string redis_str = "hset " + str1.substr(2) + " name " + login_name;

                redisReply *r = (redisReply*)redisCommand(redis_target, redis_str.c_str());

                //添加生存时间
                std::string ttl_str = "expire " + str1.substr(2) + " 300";

                r = (redisReply*)redisCommand(redis_target, ttl_str.c_str());

                std::cout << "随机生成的sessionid为:" << str1.substr(2) << std::endl; 

                send(connfd, str1.c_str(), sizeof(str1), 0);
            }
            else{
                std::cout << "登录密码错误\n\n";
                char str1[100] = "wrong";
                send(connfd, str1, sizeof(str1), 0);
            }
        
        }
        else{
            std::cout << "查询失败\n\n";
            char str1[100] = "wrong";
            send(connfd, str1, sizeof(str1), 0);
        }

    }
    //设定目标的文件描述符
    else if(str.find("target:") != str.npos)
    {
        std::cout << "私聊目标设置方法\n";
        int p1 = str.find("from");
        int key1_len=strlen("target:"),key2_len=strlen("from");
        std::string target = str.substr(key1_len, p1 - key1_len), from = str.substr(p1 + key2_len);
        target_name = target;

        if(name_sock_map.find(target) == name_sock_map.end())
        {
            std::cout << "登录用户为" << login_name 
            << ",目标用户" << target_name << "仍未登录或者不存在，无法发起私聊\n";
        }
        else{
            
            pthread_mutex_lock(&from_to_mutex);
            from_to_map[from] = target;

            pthread_mutex_unlock(&from_to_mutex);
            login_name=from;
            std::cout << "登录用户为" << login_name 
            << "向目标用户" << target_name << "发起的私聊即将建立\n";

            std::cout << ",目标用户的套接字描述符为" << name_sock_map[target_name] << std::endl;
            target_connfd = name_sock_map[target_name];
        }

    }
    //接收到消息，转发
    else if(str.find("content:") != str.npos)
    {
        std::cout << "私聊方法\n";
        target_connfd = -1;
        
        //根据map找出当前用户和目标用户
        for(auto &i : name_sock_map)
        {
            if(i.second == connfd)
            {
                login_name = i.first;
                target_name = from_to_map[i.first];
                target_connfd = name_sock_map[target_name];
                break;
            }
        }
        if(target_connfd == -1){
            std::cout<<"找不到目标用户"<<target_name<<"的套接字，将尝试重新寻找目标用户的套接字\n";
            if(name_sock_map.find(target_name) != name_sock_map.end()){
                target_connfd = name_sock_map[target_name];
                std::cout<<"重新查找目标用户套接字成功\n";
            }
            else{
                std::cout<<"查找仍然失败，转发失败！\n";
            }

        }
        std::string recv_str(str);
        std::string send_str=recv_str.substr(8);

        std::cout << "用户" << login_name << "向" << target_name
            <<"发送:" << send_str << std::endl;
        send_str="["+login_name+"]:"+send_str;

        send(target_connfd, send_str.c_str(), send_str.length(), 0);
        
    }
    //设定目标的群聊号
    else if(str.find("group:") != str.npos)
    {
        std::cout << "设置群聊号方法\n";
        int p1 = str.find("group:");
        int key1_len=strlen("group:");
        std::string group_numstr = str.substr(p1 + key1_len, str.length() - key1_len);
        group_num = stoi(group_numstr);


        //找出当前用户
        for(auto &i: name_sock_map)
        {
            if(i.second == connfd)
            {
                login_name = i.first;
                break;
            }
        }
        std::cout << "用户" << login_name << "加入群号为:" << group_num << std::endl;

        pthread_mutex_lock(&group_mutex);

        group_map[group_num].insert(connfd);

        pthread_mutex_unlock(&group_mutex);

    }
    //群聊消息
    else if(str.find("crowd_chat:") != str.npos)
    {
        std::cout << "群聊信息方法\n";
        //找出当前用户
        for (auto i : name_sock_map)
        {
            if (i.second == connfd)
            {
                login_name = i.first;
                break;
            }
        }
        //找出群号
        for(auto i:group_map)
        {
            if(i.second.find(connfd) != i.second.end()){
                group_num=i.first;
                break;
            }
        }
        int p1 = str.find("crowd_chat:");
        int key1_len = strlen("crowd_chat:");
        std::string send_to_group = str.substr(p1 + key1_len, str.length() - key1_len);
        send_to_group = "["+login_name+"]:" + send_to_group;
        std::cout << "群聊信息：" << send_to_group << std::endl;
        for(auto & i : group_map[group_num])
        {
            if(i != connfd)
            {
                send(i, send_to_group.c_str(), sizeof(send_to_group), 0);
            }
        }
    }
    
    //线程工作完成后重新注册事件
    epoll_event event;
    event.data.fd = connfd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, &event);



     mysql_close(con);
    if(!redis_target->err)
        redisFree(redis_target);
    
    //更新实参
        std::get<0>(info) = login_state;//记录当前服务对象是否成功登录
        std::get<1>(info) = login_name;//记录当前服务对象的名字
        std::get<2>(info) = target_name;//记录目标对象的名字
        std::get<3>(info) = target_connfd;//目标对象的套接字描述符
        std::get<4>(info) = group_num;//记录当前服务对象的群聊号

}




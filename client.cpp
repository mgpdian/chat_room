#include "client.h"

my_client::my_client(std::string ip, int port) : my_server_ip(ip), my_server_port(port) {}

my_client::~my_client()
{
    close(my_clientfd);
}

void my_client::run()
{
    int my_clientfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(my_server_port);
    inet_pton(AF_INET, my_server_ip.c_str(), &address.sin_addr.s_addr);

    int ret = connect(my_clientfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);
    std::cout << "连接服务器成功\n";

    HandleClient(my_clientfd);

    // std::thread send_t(SendMsg, my_clientfd);
    // std::thread recv_t(RecvMsg, my_clientfd);
    // send_t.join();
    // recv_t.join();

    close(my_clientfd);

    return;
}

//发送线程的方法
void my_client::SendMsg(int connfd)
{
    //通过connfd 
    //char sendbuffer[MAX_EVENT_NUMBER];
    while (1)
    {
        
        std::string sendbuffer;
        //memset(&sendbuffer, 0, sizeof(sendbuffer));
        std::cin >> sendbuffer;
        //如果connfd > 0 表示 私聊
        if(connfd > 0){
            sendbuffer = "content:" + sendbuffer;
        }   
        else if(connfd < 0)
        {
            
            sendbuffer = "crowd_chat:" + sendbuffer;
        }
       int ret = send(abs(connfd), sendbuffer.c_str(), sizeof(sendbuffer), 0);
        if (sendbuffer == "content:exit" || ret <= 0)
        {
            std::cout << "退出";
            break;
        }
    }
}
//接收线程的方法
void my_client::RecvMsg(int connfd)
{
    char recvbuffer[MAX_EVENT_NUMBER];
    while (1)
    {
        memset(&recvbuffer, 0, sizeof(recvbuffer));
        int ret = recv(connfd, &recvbuffer, sizeof(recvbuffer), 0);
        if (ret <= 0)
        {
            std::cout << "已退出服务器" << std::endl;
            break;
        }
        std::cout << "服务器回复" << recvbuffer << std::endl;
    }
}

void my_client::HandleClient(int connfd)
{
    
    int choice;
    std::string name, pass, pass_check;
    bool login_state = false;//登录状态 成功 或者 失败
    std::string login_name;//登录成功的用户名
    int local_login_status = false; //通过这个来确定你这次是不是登录还是注册 防止因为cookie导致想注册而不能注册的情况

     //发送本地cookie，并接收服务器答复，如果答复通过就不用登录
    //先检查是否存在cookie文件
    std::ifstream f("cookie.txt");
    std::string cookie_str;
    if(f.good())
    {
        f >> cookie_str;
        f.close();
        cookie_str = "cookie:" + cookie_str;
        //向服务器查询 是否过期了
        send(connfd, cookie_str.c_str(), sizeof(cookie_str), 0);

        //接收服务器回复
        char cookie_str[100];
        memset(cookie_str, 0, sizeof(cookie_str));
        recv(connfd, cookie_str, sizeof(cookie_str), 0);

        std::string ans_str(cookie_str);
        if(ans_str != "NULL")
        {
            login_state = true;
            login_name = ans_str;
        }
    }


    std::cout << " ------------------\n";
    std::cout << "|                  |\n";
    std::cout << "| 请输入你要的选项:|\n";
    std::cout << "|    0:退出        |\n";
    std::cout << "|    1:登录        |\n";
    std::cout << "|    2:注册        |\n";
    std::cout << "|                  |\n";
    std::cout << " ------------------ \n\n";

    //处理事务
    while (1)
    {
         if(login_state && local_login_status){
             break;
         }
        std::cout << "| 请输入你要的选项:|\n";
        std::cin >> choice;

        switch(choice)
        {
            case 0:{
                    login_state = false;
                    break;
                }
                
            case 1:
                {
                    if(login_state)
                    {
                        local_login_status = true;
                        break;
                    }
                    while(1)
                    {
                        std::cout << "用户名:";
                        std::cin >> name;
                        std::cout << "密码:";
                        std::cin >> pass;
                        name = "name:" + name;
                        pass = "pass:" + pass;
                        std::string str = "login:" + name + pass;
                        send(connfd, str.c_str(), sizeof(str), 0);
                        char buffer[1000];
                        memset(buffer, 0, sizeof(buffer));
                        recv(connfd, buffer, sizeof(buffer), 0);
                        std::string recv_buf(buffer);
                        if(recv_buf.substr(0, 2) == "OK"){
                            login_state = true;
                            local_login_status = true;
                            login_name = name;

                            //将接收的session保存到cookie文件中
                            std::string cookie_strid = recv_buf.substr(2);
                            cookie_strid = "cat > cookie.txt << end \n" + cookie_strid + "\nend"; 
                            system(cookie_strid.c_str());
                            
                            std::cout << "登录成功\n\n";
                            break;
                        }else{
                            std::cout << "账号或者密码错误\n\n";
                        }
                    }
                    continue;
                }
            case 2:{
                    std::cout << "用户名:";
                    std::cin >> name;
                    while (1)
                    {
                        std::cout << "密码:";
                        std::cin >> pass;
                        std::cout << "确认密码";
                        std::cin >> pass_check;
                        if (pass != pass_check)
                        {
                            std::cout << "密码不一致, 请重新输入";
                            continue;
                        }   
                        else
                        {

                            break;
                        }
                    }
                    name = "name:" + name;
                    pass = "pass:" + pass;
                    std::string str = "register:" + name + pass;
                    send(connfd, str.c_str(), sizeof(str), 0);
                    std::cout << "注册成功！\n";
                    
                    continue;
                }                
            default:
                std::cout << "请输入正确的选项" << std::endl;
                continue;
        }
    }


    //登录成功
    
    while (login_state)
    {
        if(login_state){
            system("clear");//清空终端d
            std::cout<<"        欢迎回来,"<< login_name<< std::endl;
            std::cout<<" -------------------------------------------\n";
            std::cout<<"|                                           |\n";
            std::cout<<"|          请选择你要的选项：               |\n";
            std::cout<<"|              0:退出                       |\n";
            std::cout<<"|              1:发起单独聊天               |\n";
            std::cout<<"|              2:发起群聊                   |\n";
            std::cout<<"|                                           |\n";
            std::cout<<" ------------------------------------------- \n\n";
        }
        std::cin >> choice;
        if(choice == 0)
            break;
        
        //1 私聊
        if(choice == 1)
        {
            std::cout << "请输入对方的用户名:";
            std::string target_name, content;
            std::cin >> target_name;

            //记录私聊的双方名字
            std::string private_names("target:" + target_name + "from:" + name);

            send(connfd, private_names.c_str(), sizeof(private_names), 0);
            std::cout << "请输入你想说的话(输入exit退出):\n";
            std::thread t1(SendMsg, connfd);//创建发送线程
            std::thread t2(RecvMsg, connfd);//创建接收线程
            t1.join();
            t2.join();
        }
        //2 群聊
        else if(choice == 2)
        {
            std::cout << "请输入群号:";
            int num;
            std::cin >> num;
            
            //记录群号
            std::string group_number("group:" + std::to_string(num));
            std::cout << group_number;
            send(connfd, group_number.c_str(), sizeof(group_number), 0);
            std::cout << "请输入你想说的话(输入exit退出):\n";
            std::thread t1(SendMsg, -connfd); //创建发送线程 使用负号来区分 私聊和群聊
            std::thread t2(RecvMsg, connfd); //创建接收线程
            t1.join();
            t2.join();

        }
        
    }
    close(connfd);
}

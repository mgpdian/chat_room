#include"server.h"
int main(){
    my_server serv("127.0.0.1", 9999);//创建实例，传入端口号和ip作为构造函数参数
    serv.run();//启动服务
}
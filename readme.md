# 聊天室笔记

# Linux聊天室项目

### 项目介绍

使用c++编程语言，使用面向对象封装，实现网络聊天室。

网络聊天室具有注册，登录，单聊，群聊，cookie记住登陆状态等功能。

### 项目文件介绍

```
//公共的头文件
global.h
//聊天室客户端的类
client.h
client.cpp
//聊天室服务端的类
server.h
server.cpp
//实例化客户端，生成客户端程序
test_client.cpp
//实例化服务端，生成服务端程序
test_server.cpp
//代码编译
makefile
```

### 不重要的文件

```
//测试redis时使用
test_redis.cpp
//程序运行中自动生成的文件
cookie.txt
```

### 编译

编译前安装的库

```
# 安装mysql(可选)
sudo apt-get install mysql-server 
sudo apt-get install mysql-client 
sudo apt-get install libmysqlclient-dev

# 安装mariadb，mysql的替代品
sudo apt-get install mariadb-server 
sudo apt-get install mariadb-client
sudo apt-get install libmariadbclient-dev

#安装redis
sudo apt-get install redis-server

# 安装hiredis，c语言连接库
sudo apt search hiredis
```

安装完数据库后需要创建表

编译

```
make
```

服务端运行

```
./test_server
```

客户端运行

```
./test_client
```

感想

1 首先一个简单的服务器和客户端

还是写了很久 东西都忘了 

2 开始多线程

使用了c++11的thread线程库 而不是Linux 的pthread_create

开始进行封装

3 改进客户端 让他能进行接受和发送同时进行

考虑到server和client的头文件使用基本相同 使用一个h类将头文件全部包含

4 开始使用mysql 来登录注册

5 完成私聊和群聊

6 通过redis实现记录用户登录状态

> 注：下文提到的 cookie 都等价于 sessionid
>
> 利用 Redis 记录用户登录状态（HASH 类型，键为 sessionid，值为 session 对象，键五分钟后过期），当用户成功登录时服务器会利用随机算法生成 sessionid 发送到客户端保存，客户登录时会优先发送 cookie 到服务器检查，如果检查通过就不用输入账号密码登录。
>
> 本次实验先实现服务器端的功能，要求如下：
>
> - 服务器收到账号密码后先查询 mysql 看看是否正确，如果正确则成功登录，并生成一个随机数作为 sessionid，然后往 Redis 插入一条数据（key 为 sessionid，value 为字典类型（字典中存储的键值为“name”，实值为用户名），并设置过期时间为 300 秒），然后将 sessionid 发给客户端，客户端收到后将其存到本地的 cookie.txt 中。
> - 如果用户本地已经有 cookie 文件了，登录时会先把 cookie 发给服务器进行校验，服务器需要查询 Redis 是否存在 key 为该 cookie，如果存在那就可以通知客户端跳过登录步骤，否则通知客户端继续输入账号密码。
>
> ### 具体设计思路
>
> 假设用户 xiaoming 登录，服务器随机生成的 sessionid 为 1a2b3c4DEF，那么会执行如下的 Redis 插入语句：`hset 1a2b3c4DEF name xiaoming`，然后执行如下语句设置过期时间为 300 秒：`expire 1a2b3c4DEF 300` ，服务器将该 sessionid 发往客户端作为 cookie 保存，客户端再重新启动进程会先将 cookie 发往服务器，服务器收到客户端发来的 sessionid 后查询 Redis，使用如下语句：`hget 1a2b3c4DEF name`，只要该 sessionid 还未过期，就可以查询到结果，告知客户端登录成功以及用户名。
>
> 随机生成 sessionid（也就是 cookie）时我们会采用如下的算法：
>
> sessionid 大小为 10 位，每位由数字（0-9）、小写字母、大写字母随机组成，理论上有(10+26+26)^10 种组合，很难出现重复

客户端 保存cookie的方式是保存到本地文件中

7 使用epoll和boost库的线程池和处理事件的Reactor模式实现的并发服务器 epoll采用ET边缘触发

基本重做

项目很差 不能拿出来看  不论没有说明线程池的原理还是 连接断开时没有进行处理的各种情况 

万幸的是 学到了redis的基本连接操作 这是比较好的消息
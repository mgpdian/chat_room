#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <iostream>
#include <thread>
#include <vector>
#include <assert.h>
#include <mysql/mysql.h>
#include <unordered_map>
#include <pthread.h>
#include <set>
#include <hiredis/hiredis.h> //连接redis的文件
#include <fstream>

#include <sys/epoll.h>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <errno.h>
//全局要使用的头文件
const int MAX_EVENT_NUMBER = 10000;
#endif
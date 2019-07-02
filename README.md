# file-system
编译命令：

客户端：

g++ AnalysisMessage.cpp client.cpp ClientWork.cpp -o cli -lmuduo_net -lmuduo_base -lpthread -std=c++11 -g

服务器：

g++ server.cpp ServerWork.cpp MySql.cpp AnalysisMessage.cpp -o ser1 -lboost_coroutine -lmuduo_net -lmuduo_base -lpthread -lmysqlclient  -I/usr/include/mysql -L/usr/lib/mysql -std=c++11 -g

负载均衡器

g++ balance.cpp balanceWork.cpp  AnalysisMessage.cpp -o balance -lmuduo_net -lmuduo_base -lpthread -std=c++11 -g

运行事例：

./balance

./ser1 8000 9000

./cli 127.0.0.1 8888

运行条件：

1、当前问目录下必须要有一个file目录

2、必须要有相应的数据库表

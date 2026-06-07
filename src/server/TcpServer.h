#pragma once
#include"server/Connection.h"
#include<sys/epoll.h>
#include<vector>
#include<unordered_map>
#include<memory>

class TcpServer{
public:
	TcpServer(int port,int maxEvents=64);
	~TcpServer();

	void run();

private:
	void initSocket();
	void initEpoll();
	void handleNewConnection();
	void handleClientData(int clientFd);
	void setNonBlocking(int fd);
	void closeConnection(int clientFd);

	//消息分支：根据消息类型调用对应处理函数
	void dispatchMessage(Connection& conn,uint16_t msgType,const std::string& body);

	//具体消息处理函数
	void onLoginRequest(Connection& conn,const std::string& body);

	//发送一条protobuf消息给客户端
	void sendMessage(int clientFd,uint16_t msgType,const std::string& body);

	int port_;
	int listenFd_;
	int epollFd_;
	int maxEvents_;
	std::vector<epoll_event> events_;

	//每个连接fd对应一个Connection
	std::unordered_map<int,std::shared_ptr<Connection>> connections_;
};

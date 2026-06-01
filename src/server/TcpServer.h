#pragma once
#include"server/Buffer.h"
#include<sys/epoll.h>
#include<vector>
#include<unordered_map>

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

	int port_;
	int listenFd_;
	int epollFd_;
	int maxEvents_;
	std::vector<epoll_event> events_;

	//每个连接fd对应一个Buffer
	std::unordered_map<int,Buffer> buffers_;
};

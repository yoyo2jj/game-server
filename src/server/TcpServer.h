#pragma once
#include"server/Connection.h"
#include"server/RoomManager.h"
#include"server/ThreadPool.h"
#include<sys/epoll.h>
#include<vector>
#include<unordered_map>
#include<memory>
#include<mutex>

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

	void onHeartbeat(Connection& conn,const std::string& body);

	void onCreateRoomRequest(Connection& conn,const std::string& body);

	void onJoinRoomRequest(Connection& conn,const std::string& body);

	//广播消息给房间内所有玩家
	void broadcastToRoom(int roomId,uint16_t msgType,const std::string& body);

	//发送一条protobuf消息给客户端
	void sendMessage(int clientFd,uint16_t msgType,const std::string& body);

	//检查所有连接，踢掉超时的
	void checkHeartbeat();

	int port_;
	int listenFd_;
	int epollFd_;
	int maxEvents_;
	std::vector<epoll_event> events_;

	//保护connections_的锁，工作线程会访问它
	std::mutex connMutex_;

	ThreadPool threadPool_;

	//每个连接fd对应一个Connection
	std::unordered_map<int,std::shared_ptr<Connection>> connections_;

	RoomManager roomManager_;

	//超时阈值：60秒没有数据就踢掉
	static const int HEARTBEAT_TIMEOUT=60;
	//每10秒检查一次
	static const int CHECK_INTERVAL=10;
};

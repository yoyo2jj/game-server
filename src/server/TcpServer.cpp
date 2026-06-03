#include"server/TcpServer.h"
#include"proto/game.pb.h"
#include<iostream>
#include<cstring>
#include<stdexcept>
#include<unistd.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/epoll.h>

TcpServer::TcpServer(int port,int maxEvents)
	:port_(port),listenFd_(-1),epollFd_(-1),maxEvents_(maxEvents){
	events_.resize(maxEvents_);
	initSocket();
	initEpoll();
}

TcpServer::~TcpServer(){
	if(listenFd_!=-1) close(listenFd_);
	if(epollFd_!=-1) close(epollFd_);
}

void TcpServer::setNonBlocking(int fd){
	int flags=fcntl(fd,F_GETFL,0);
	fcntl(fd,F_SETFL,flags|O_NONBLOCK);
}

void TcpServer::initSocket(){
	listenFd_=socket(AF_INET,SOCK_STREAM,0);
	if(listenFd_<0) throw std::runtime_error("socket() failed");

	//端口重用，避免重启服务器时bind失败
	int opt=1;
	setsockopt(listenFd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	sockaddr_in addr{};
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port_);
	addr.sin_addr.s_addr=INADDR_ANY;

	if(bind(listenFd_,(sockaddr*)&addr,sizeof(addr))<0)
		throw std::runtime_error("bind() failed");

	if(listen(listenFd_,SOMAXCONN)<0)
		throw std::runtime_error("listen() failed");

	setNonBlocking(listenFd_);
	std::cout<<"[Server] Listening on port"<<port_<<std::endl;
}

void TcpServer::initEpoll(){
	epollFd_=epoll_create1(0);
	if(epollFd_<0) throw std::runtime_error("epoll_create1() failed");

	//把监听fd注册到epoll，ET模式
	epoll_event ev{};
	ev.events=EPOLLIN|EPOLLET;
	ev.data.fd=listenFd_;
	epoll_ctl(epollFd_,EPOLL_CTL_ADD,listenFd_,&ev);
}

void TcpServer::handleNewConnection(){
	//ET模式下必须循环accept，直到EAGAIN
	while(true){
		sockaddr_in clientAddr{};
		socklen_t clientLen=sizeof(clientAddr);
		int clientFd=accept(listenFd_,(sockaddr*)&clientAddr,&clientLen);
		if(clientFd<0){
			if(errno==EAGAIN||errno==EWOULDBLOCK) break;
			std::cerr<<"[Server] accept() error"<<std::endl;
			break;
		}

		setNonBlocking(clientFd);

		//把新客户端fd注册到epoll，ET模式
		epoll_event ev{};
		ev.events=EPOLLIN|EPOLLET;
		ev.data.fd=clientFd;
		epoll_ctl(epollFd_,EPOLL_CTL_ADD,clientFd,&ev);

		std::cout<<"[Server] New connection, fd="<<clientFd<<std::endl;
		buffers_[clientFd];
	}
}

void TcpServer::handleClientData(int clientFd){
	char buf[1024];
	Buffer& buffer=buffers_[clientFd];

	//ET模式下必须循环read，直到EAGAIN
	while(true){
		ssize_t n=read(clientFd,buf,sizeof(buf));
		if(n<0){
			if(errno==EAGAIN||errno==EWOULDBLOCK) break;//数据读完了
			std::cerr<<"[Server] read() error, fd="<<clientFd<<std::endl;
			epoll_ctl(epollFd_,EPOLL_CTL_DEL,clientFd,nullptr);
			close(clientFd);
			buffers_.erase(clientFd);
			break;
		}else if(n==0){
			//客户端断开连接
			std::cout<<"[Server] Client disconnected, fd="<<clientFd<<std::endl;
			epoll_ctl(epollFd_,EPOLL_CTL_DEL,clientFd,nullptr);
			close(clientFd);
			buffers_.erase(clientFd);
			return;
		}else{
			buffer.append(buf,n);
		}
	}

	//循环读出所有完整消息
	uint16_t msgType;
	std::string body;
	while(buffer.retrieveMessage(msgType,body)){
		dispatchMessage(clientFd,msgType,body);
	}
}

void TcpServer::dispatchMessage(int clientFd,uint16_t msgType,const std::string& body){
	//根据消息类型，分发到对应处理函数
	switch(msgType){
		case 1://MSG_LOGIN_REQUEST
			onLoginRequest(clientFd,body);
			break;
		default:
			std::cerr<<"[Server] Unknown msgType="<<msgType<<std::endl;
			break;
	}
}

void TcpServer::onLoginRequest(int clientFd,const std::string& body){
	//反序列化protobuf消息
	game::LoginRequest req;
	if(!req.ParseFromString(body)){
		std::cerr<<"[Server] Failed to parse LoginRequest"<<std::endl;
		return;
	}

	std::cout<<"[Server] LoginRequest from fd="<<clientFd
		<<" username="<<req.username()<<std::endl;

	//简单逻辑：用户名不为空就登录成功
	game::LoginResponse resp;
	if(!req.username().empty()){
		resp.set_success(true);
		resp.set_message("Welcome, "+req.username()+"!");
	}else {
		resp.set_success(false);
		resp.set_message("Username cannot be empty");
	}

	//序列化并发送
	std::string respBody;
	resp.SerializeToString(&respBody);
	sendMessage(clientFd,2,respBody);//2=MSG_LOGIN_RESPONSE
}

void TcpServer::sendMessage(int clientFd,uint16_t msgType,const std::string& body){
	//总长度=2字节消息类型+body长度
	uint32_t totalLen=2+body.size();

	//按协议格式拼好再一次性发出
	std::string packet;
	packet.resize(4+2+body.size());
	memcpy(&packet[0],&totalLen,4);
	memcpy(&packet[4],&msgType,2);
	memcpy(&packet[6],body.c_str(),packet.size());
	
	write(clientFd,packet.c_str(),packet.size());
}

void TcpServer::run(){
	std::cout<<"[Server] Running..."<<std::endl;
	while(true){
		int nfds=epoll_wait(epollFd_,events_.data(),maxEvents_,-1);
		for(int i=0;i<nfds;i++){
			int fd=events_[i].data.fd;
			if(fd==listenFd_){
				handleNewConnection();
			}else{
				handleClientData(fd);
			}
		}
	}
}

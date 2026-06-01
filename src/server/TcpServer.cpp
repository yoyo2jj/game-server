#include"server/TcpServer.h"
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

	//尝试从Buffer里取出完整消息
	std::string message;
	while(buffer.retrieveMessage(message)){
		std::cout<<"[Server] Got message: "<<message<<std::endl;

		//原样echo回去，带上包头
		uint32_t bodyLen=message.size();
		write(clientFd,&bodyLen,4);
		write(clientFd,message.c_str(),bodyLen);
	}
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

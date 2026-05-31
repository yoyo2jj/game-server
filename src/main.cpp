#include"server/TcpServer.h"
#include<iostream>

int main(){
	try{
		TcpServer server(8888);
		server.run();
	}catch(const std::exception& e){
		std::cerr<<"[Fatal] "<<e.what()<<std::endl;
		return 1;
	}
	return 0;
}

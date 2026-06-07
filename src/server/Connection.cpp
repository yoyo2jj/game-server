#include "server/Connection.h"

Connection::Connection(int fd)
	:fd_(fd),isLoggedIn_(false),roomId_(-1){
	lastActiveTime_=std::chrono::steady_clock::now();
}

void Connection::updateActiveTime(){
	lastActiveTime_=std::chrono::steady_clock::now();
}

int Connection::idleSeconds() const{
	auto now=std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::seconds>(
			now-lastActiveTime_).count();
}

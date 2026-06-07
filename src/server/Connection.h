#pragma once
#include"server/Buffer.h"
#include<string>

class Connection{
public:
	explicit Connection(int fd);

	int fd() const{ return fd_; }

	//Buffer 相关
	Buffer& buffer() { return buffer_; }

	//登录状态
	bool isLoggedIn() const { return isLoggedIn_; }
	void setLoggedIn(bool v) { isLoggedIn_=v; }

	//用户名
	const std::string& username() const { return username_; }
	void setUsername(const std::string& name) { username_=name; }

	//房间ID，-1表示不在任何房间
	int roomId() const { return roomId_; }
	void setRoomId(int id) { roomId_=id; }

private:
	int fd_;
	Buffer buffer_;
	bool isLoggedIn_=false;
	std::string username_;
	int roomId_=-1;
};
	

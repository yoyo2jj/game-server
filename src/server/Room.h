#pragma once
#include<vector>
#include<string>

class Room{
public:
	explicit Room(int roomId);

	int roomId() const { return roomId_; }

	//房间是否已满（最多2人）
	bool isFull() const { return playerFds_.size()>=2;}

	//房间是否为空
	bool isEmpty() const { return playerFds_.empty(); }

	//加入房间，返回是否成功
	bool addPlayer(int fd,const std::string& username);

	//离开房间
	void removePlayer(int fd);

	//获取房间内所有玩家的fd
	const std::vector<int>& playerFds() const { return playerFds_; }

	//获取房间内所有玩家的用户名
	const std::vector<std::string>& usernames() const{ return usernames_; }

private:
	int roomId_;
	std::vector<int> playerFds_;
	std::vector<std::string> usernames_;
};

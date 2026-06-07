#include"server/Room.h"
#include<algorithm>

Room::Room(int roomId):roomId_(roomId){}

bool Room::addPlayer(int fd,const std::string& username){
	if(isFull()) return false;
	playerFds_.push_back(fd);
	usernames_.push_back(username);
	return true;
}

void Room::removePlayer(int fd){
	for(int i=0;i<(int)playerFds_.size();i++){
		if(playerFds_[i]==fd){
			playerFds_.erase(playerFds_.begin()+i);
			usernames_.erase(usernames_.begin()+i);
		}
	}
}

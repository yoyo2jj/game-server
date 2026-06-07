#pragma once
#include "server/Room.h"
#include<unordered_map>
#include<memory>

class RoomManager{
public:
	//创建新房间，返回房间ID
	int createRoom();

	//根据ID查找房间，找不到返回nullptr
	Room* findRoom(int roomId);

	//玩家离开房间，房间空了就删掉
	void leaveRoom(int roomId,int playerFd);

private:
	std::unordered_map<int,std::shared_ptr<Room>> rooms_;
	int nextRoomId_=1;
};

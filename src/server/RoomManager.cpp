#include "server/RoomManager.h"

int RoomManager::createRoom(){
	int roomId=nextRoomId_++;
	rooms_[roomId]=std::make_shared<Room>(roomId);
	return roomId;
}

Room* RoomManager::findRoom(int roomId){
	auto it=rooms_.find(roomId);
	if(it==rooms_.end()) return nullptr;
	return it->second.get();
}

void RoomManager::leaveRoom(int roomId,int playerFd){
	Room* room=findRoom(roomId);
	if(!room) return;
	room->removePlayer(playerFd);
	//房间空了就删掉
	if(room->isEmpty()){
		rooms_.erase(roomId);
	}
}

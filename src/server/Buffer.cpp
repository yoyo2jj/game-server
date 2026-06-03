#include"server/Buffer.h"
#include<cstring>

void Buffer::append(const char* data,size_t len){
	//把收到的字节追加到缓冲区末尾
	buffer_.insert(buffer_.end(),data,data+len);
}

size_t Buffer::readableBytes() const{
	return buffer_.size();
}

bool Buffer::retrieveMessage(uint16_t& msgType, std::string& body){
	//数据不够4字节，包头都读不完，直接返回
	if(buffer_.size()<HEADER_SIZE) return false;

	//读包头，拿到包体总长度（包含2字节消息类型）
	uint32_t totalBodyLen=0;
	memcpy(&totalBodyLen,buffer_.data(),HEADER_SIZE);

	//包头+包体都到齐了才算一条完整信息
	if(buffer_.size()<HEADER_SIZE+totalBodyLen) return false;

	//读消息类型（包头后面紧跟2字节）
	memcpy(&msgType, buffer_.data()+HEADER_SIZE,TYPE_SIZE);

	//剩下的是protobuf数据
	uint32_t protoLen=totalBodyLen-TYPE_SIZE;
	body.assign(buffer_.begin()+HEADER_SIZE+TYPE_SIZE,
			buffer_.begin()+HEADER_SIZE+TYPE_SIZE+protoLen);

	//从缓冲区移除这条完整信息
	buffer_.erase(buffer_.begin(),buffer_.begin()+HEADER_SIZE+totalBodyLen);

	return true;
}

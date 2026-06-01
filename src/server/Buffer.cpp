#include"server/Buffer.h"
#include<cstring>

void Buffer::append(const char* data,size_t len){
	//把收到的字节追加到缓冲区末尾
	buffer_.insert(buffer_.end(),data,data+len);
}

size_t Buffer::readableBytes() const{
	return buffer_.size();
}

bool Buffer::retrieveMessage(std::string& out){
	//数据不够4字节，包头都读不完，直接返回
	if(buffer_.size()<HEADER_SIZE) return false;

	//读包头，拿到包体长度
	uint32_t bodyLen=0;
	memcpy(&bodyLen,buffer_.data(),HEADER_SIZE);

	//包头+包体都到齐了才算一条完整信息
	if(buffer_.size()<HEADER_SIZE+bodyLen) return false;

	//提取包体内容
	out.assign(buffer_.begin()+HEADER_SIZE,buffer_.begin()+HEADER_SIZE+bodyLen);

	//从缓冲区移除这条信息（包头+包体）
	buffer_.erase(buffer_.begin(),buffer_.begin()+HEADER_SIZE+bodyLen);

	return true;
}

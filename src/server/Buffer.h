#pragma once
#include<vector>
#include<string>
#include<cstdint>

//每个连接一个Buffer，负责积累字节、提取完整信息
class Buffer{
public:
	//收到数据后调用，把数据追加进来
	void append(const char* data,size_t len);

	//尝试从缓冲区取出一条完整信息，写入out
	//成功返回true，数据不够返回false
	bool retrieveMessage(std::string& out);

	//当前缓冲区里有多少字节
	size_t readableBytes() const;

private:
	std::vector<char> buffer_;

	//包头固定4字节，存的是包体长度
	static const int HEADER_SIZE=4;
};


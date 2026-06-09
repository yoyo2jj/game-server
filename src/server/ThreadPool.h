#pragma once
#include<vector>
#include<queue>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<atomic>

class ThreadPool{
public:
	explicit ThreadPool(int numThreads);
	~ThreadPool();

	//提交一个任务到线程池
	void submit(std::function<void()> task);

private:
	void workerLoop();

	std::vector<std::thread> workers_;
	std::queue<std::function<void()>> taskQueue_;
	std::mutex mutex_;
	std::condition_variable cv_;
	std::atomic<bool> stop_;
};

#include "server/ThreadPool.h"

ThreadPool::ThreadPool(int numThreads):stop_(false){
	for(int i=0;i<numThreads;i++){
		workers_.emplace_back(&ThreadPool::workerLoop,this);
	}
}

ThreadPool::~ThreadPool(){
	stop_=true;
	cv_.notify_all();
	for(auto& t:workers_){
		if(t.joinable()) t.join();
	}
}

void ThreadPool::submit(std::function<void()> task){
	{
		std::lock_guard<std::mutex> lock(mutex_);
		taskQueue_.push(std::move(task));
	}
	//通知一个工作线程有新任务
	cv_.notify_one();
}

void ThreadPool::workerLoop(){
	while(true){
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(mutex_);
			//没有任务就阻塞等待，直到有任务或者stop
			cv_.wait(lock,[this]{
				return !taskQueue_.empty()||stop_;
			});

			if(stop_&&taskQueue_.empty()) return;

			task=std::move(taskQueue_.front());
			taskQueue_.pop();
		}
		//执行任务，锁已经释放
		task();
	}
}

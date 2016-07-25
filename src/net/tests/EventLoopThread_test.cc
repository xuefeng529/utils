#include "net/EventLoop.h"
#include "net/EventLoopThread.h"

#include <boost/bind.hpp>
#include <iostream>

using namespace net;

void worker(const std::string& task)
{
	std::cout << task << std::endl;
}

int main(int argc, char* argv[])
{
	EventLoop mainLoop;

	EventLoopThread loopThread;
	std::cout << "befor loopThread.startLoop" << std::endl;
	EventLoop* loop = loopThread.startLoop();
	(void)loop;
	std::cout << "after loopThread.startLoop" << std::endl;
	loop->runInLoop(boost::bind(worker, "working"));
	
	mainLoop.loop();
	return 0;
}
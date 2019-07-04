#include "base/TimingWheel.h"
#include "base/Timestamp.h"

#include <iostream>
#include <boost/bind.hpp>

void onTimer(int timeout, int id)
{
	std::cout << "timeout: " << timeout << ", id: " << id << ", now: " 
		<< base::Timestamp::now().toFormattedString(false)<< std::endl;
}

int main()
{
	std::cout << "now: " << base::Timestamp::now().toFormattedString(false) << std::endl;

	base::TimingWheel timer(1000, 60);
	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 1, i), 1000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return 0;
		}
	}
	
	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 3, i), 3000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return 0;
		}
	}
	
	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 5, i), 5000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return 0;
		}
	}
	
	timer.start();

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 15, i), 15000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return 0;
		}
	}
	
	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 25, i), 25000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return 0;
		}
	}

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 35, i), 35000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return 0;
		}
	}

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 60, i), 60000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return 0;
		}
	}

	std::string line;
	std::cin >> line;
	return 0;
}

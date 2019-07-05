#include "base/TimingWheel.h"
#include "base/Timestamp.h"

#include <iostream>
#include <boost/bind.hpp>

base::TimingWheel timer(1000, 60);

void onTimer(int timeout, int id)
{
	std::cout << "timeout: " << timeout << ", id: " << id << ", now: " 
		<< base::Timestamp::now().toFormattedString(false)<< std::endl;
	if (id == 3)
	{
		sleep(5);
	}
}

void test1()
{
	std::cout << "now: " << base::Timestamp::now().toFormattedString(false) << std::endl;
	base::TimingWheel::TimeoutPtr timeout;

	if (!timer.addTimeout(boost::bind(onTimer, 1, 1), 1000))
	{
		std::cout << "addTimeout failed" << std::endl;
		return;
	}

	if (!timer.addTimeout(boost::bind(onTimer, 3, 3), 3000))
	{
		std::cout << "addTimeout failed" << std::endl;
		return;
	}

	timeout = timer.addTimeout(boost::bind(onTimer, 5, 5), 5000);
	if (!timeout)
	{
		std::cout << "addTimeout failed" << std::endl;
		return;
	}

	sleep(3);
	timer.start();

	if (!timer.addTimeout(boost::bind(onTimer, 15, 15), 15000))
	{
		std::cout << "addTimeout failed" << std::endl;
		return;
	}

	if (!timer.addTimeout(boost::bind(onTimer, 25, 25), 25000))
	{
		std::cout << "addTimeout failed" << std::endl;
		return;
	}

	timeout = timer.addTimeout(boost::bind(onTimer, 35, 35), 35000);
	if (!timeout)
	{
		std::cout << "addTimeout failed" << std::endl;
		return;
	}

	timeout->cancell();

	if (!timer.addTimeout(boost::bind(onTimer, 60, 60), 60000))
	{
		std::cout << "addTimeout failed" << std::endl;
		return;
	}
}

void test2()
{
	std::cout << "now: " << base::Timestamp::now().toFormattedString(false) << std::endl;
	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 1, i), 1000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return;
		}
	}

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 3, i), 3000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return;
		}
	}

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 5, i), 5000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return;
		}
	}

	timer.start();

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 15, i), 15000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return;
		}
	}

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 25, i), 25000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return;
		}
	}

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 35, i), 35000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return;
		}
	}

	for (int i = 1; i < 5; i++)
	{
		if (!timer.addTimeout(boost::bind(onTimer, 60, i), 60000))
		{
			std::cout << "addTimeout failed" << std::endl;
			return;
		}
	}
}

int main()
{
	test1();
	std::string line;
	std::cin >> line;
	timer.stop();
	return 0;
}

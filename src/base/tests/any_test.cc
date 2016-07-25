#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

class Data
{
public:
	Data()
	{
		std::cout << "Data::Data" << std::endl;
	}

	~Data()
	{
		std::cout << "Data::~Data" << std::endl;
	}

};

class Foo
{
public:
	void setContext(const boost::any& context)
	{
		context_ = context;
	}

	const boost::any& getContext() const
	{ return context_; }

	boost::any* getMutableContext()
	{ return &context_; }

private:
	boost::any context_;
};

int main()
{
	{
		Foo foo;
		boost::shared_ptr<Data> data(new Data());
		foo.setContext(data);
	}
	
	/*if (!foo.getContext().empty())
	{
	boost::shared_ptr<Data> data1 = boost::any_cast<boost::shared_ptr<Data> >(foo.getContext());
	}*/

	while (1)
	{
		usleep(1000);
	}

	return 0;
}
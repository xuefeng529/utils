#ifndef BASE_XMLPARSER_H
#define BASE_XMLPARSER_H

#include "base/Logging.h"

#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/typeof/typeof.hpp>

#include <map>
#include <vector>

namespace base
{

class XmlParser : boost::noncopyable
{
public:
	void read(const std::string& filename);

	template<typename Type>
	Type get(const std::string& path) const;

	std::string getAttr(const std::string& path, const std::string& attr) const;

	template<typename Type>
	void getChild(const std::string& path, std::vector<Type>* ret) const;

	void getChildAttrs(const std::string& path, 
					   const std::vector<std::string>& attrs, 
					   std::vector<std::map<std::string, std::string> >* ret) const;

private:
	boost::property_tree::ptree pt_;
};

void XmlParser::read(const std::string& filename)
{
	assert(!filename.empty());
	try
	{
		boost::property_tree::xml_parser::read_xml(filename, pt_);
	}
	catch (const std::exception& e)
	{
		LOG_ERROR << "parsing xml failed: " << e.what();
		abort();
	}
}

template<typename Type>
Type XmlParser::get(const std::string& path) const
{
	assert(!path.empty());
	try
	{
		return pt_.get<Type>(path);
	}
	catch (const std::exception& e)
	{
		LOG_ERROR << "parsing xml failed: " << e.what();
		abort();
	}
}

std::string XmlParser::getAttr(const std::string& path, const std::string& attr) const
{
	assert(!path.empty());
	assert(!attr.empty());
	std::string newPath = path + ".<xmlattr>." + attr;
	try
	{
		return pt_.get<std::string>(newPath);
	}
	catch (const std::exception& e)
	{
		LOG_ERROR << "parsing xml failed: " << e.what();
		abort();
	}
}

template<typename Type>
void XmlParser::getChild(const std::string& path, std::vector<Type>* ret) const
{
	assert(!path.empty());
	assert(ret != NULL);
	ret->clear();
	try
	{
		BOOST_AUTO(child, pt_.get_child(path));
		for (BOOST_AUTO(pos, child.begin()); pos != child.end(); ++pos)
		{
			ret->push_back(pos->second.get_value<Type>());
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR << "parsing xml failed: " << e.what();
		abort();
	}
}

void XmlParser::getChildAttrs(const std::string& path,
							  const std::vector<std::string>& attrs,
							  std::vector<std::map<std::string, std::string> >* ret) const
{
	assert(!path.empty());
	assert(!attrs.empty());
	assert(ret != NULL);
	ret->clear();
	try
	{
		BOOST_AUTO(child, pt_.get_child(path));
		for (BOOST_AUTO(pos, child.begin()); pos != child.end(); ++pos)
		{
			ret->push_back(std::map<std::string, std::string>());
			size_t idx = ret->size() - 1;
			for (size_t i = 0; i < attrs.size(); i++)
			{
				std::string attr = std::string("<xmlattr>.") + attrs[i];
				(*ret)[idx].insert(std::make_pair(attrs[i], pos->second.get<std::string>(attr)));
			}
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR << "parsing xml failed: " << e.what();
		abort();
	}
}

} // namespace base

#endif // BASE_XMLPARSER_H

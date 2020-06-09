/// <root>
///		<child>
///			<int>1314</int>
///			<float>3.14</float>
///			<string>hello world</string>
///			<attr title="test attr"></attr>
///			<subchild>
///				<category code="1" name="@我的" content="与您相关的评论，问答，点赞">test child1</category>
///				<category code="2" name="评论我的" content="与您相关的评论，问答，点赞">test child2</category>
///				<category code="3" name="赞我的" content="与您相关的评论，问答，点赞">test child3</category>
///			</subchild>
///		</child>
/// </root>

#include "base/XmlParser.h"

#include <iostream>

int main(int argc, char* argv[])
{
	base::XmlParser parser;
	parser.read(argv[1]);
	int i = parser.get<int>("root.child.int");
	std::cout << "test int: " << i << std::endl;
	float f = parser.get<float>("root.child.float");
	std::cout << "test float: " << f << std::endl;
	std::string s = parser.get<std::string>("root.child.string");
	std::cout << "test string: " << s << std::endl;

	std::cout << "test attr: " << parser.getAttr("root.child.attr", "title") << std::endl;

	std::vector<std::string> values;
	parser.getChild("root.child.subchild", &values);
	for (size_t i = 0; i < values.size(); ++i)
	{
		std::cout << "test subchild: " << values[i] << std::endl;
	}

	std::vector<std::string> attrs;
	attrs.push_back("code");
	attrs.push_back("name");
	attrs.push_back("content");
	std::vector<std::map<std::string, std::string> > ret;
	parser.getChildAttrs("root.child.subchild", attrs, &ret);
	for (size_t i = 0; i < ret.size(); i++)
	{
		std::map<std::string, std::string>::const_iterator it = ret[i].begin();
		for (; it != ret[i].end(); ++it)
		{
			if (it != ret[i].begin())
			{
				std::cout << ", ";
			}
	
			std::cout << it->first << ": " << it->second;
		}

		std::cout << std::endl;
	}
	
	return 0;
}

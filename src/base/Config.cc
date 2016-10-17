#include "base/Config.h"
#include "base/Logging.h"
#include "base/StringUtil.h"

#include <stdio.h>

namespace base
{

int Config::parseValue(char *str, char *key, char *val)
{
	if (str == NULL)
	{
		return -1;
	}

	char *p, *p1, *name, *value;
	p = str;
	// 去前置空格
	while (isspace(*p))
	{
		p++;
	}
	p1 = p + strlen(p);
	// 去后置空格
	while (p1 > p)
	{
		p1--;
		if (isspace(*p1))
		{
			continue;
		}
		p1++;
		break;
	}
	(*p1) = '\0';
	// 是注释行或空行
	if (*p == '#' || *p == '\0')
	{
		return -1;
	}

	p1 = strchr(str, '=');
	if (p1 == NULL)
	{
		return -2;
	}

	name = p;
	value = p1 + 1;
	while ((*(p1 - 1)) == ' ')
	{
		p1--;
	}
	(*p1) = '\0';

	while ((*value) == ' ')
	{
		value++;
	}
	p = strchr(value, '#');
	if (p == NULL)
	{
		p = value + strlen(value);
	}
	while ((*(p - 1)) <= ' ')
	{
		p--;
	}
	(*p) = '\0';
	if (name[0] == '\0')
	{
		return -2;
	}
		
	strcpy(key, name);
	strcpy(val, value);
	return 0;
}

char *Config::isSectionName(char *str)
{
	if (str == NULL || strlen(str) <= 2 || (*str) != '[')
	{
		return NULL;
	}

	char *p = str + strlen(str);
	while ((*(p - 1)) == ' ' || (*(p - 1)) == '\t' || (*(p - 1)) == '\r' || (*(p - 1)) == '\n')
	{
		p--;
	}
	if (*(p - 1) != ']')
	{
		return NULL;
	}

	*(p - 1) = '\0';

	p = str + 1;
	while (*p)
	{
		if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || (*p == '_'))
		{
		}
		else
		{
			return NULL;
		}
		p++;
	}
	return (str + 1);
}

bool Config::load(const std::string& filename)
{
	FILE *fp;
	char key[128], value[4096], data[4096];
	int ret, line = 0;
	if ((fp = fopen(filename.c_str(), "rb")) == NULL)
	{
		LOG_ERROR << "不能打开配置文件: " << filename;
		return false;
	}

	KeyValueMutimapPtr keyValueMutimap;
	while (fgets(data, 4096, fp))
	{
		line++;
		char *sName = isSectionName(data);
		// 是段名
		if (sName != NULL)
		{
			SectionMap::const_iterator it = configMap_.find(sName);
			if (it == configMap_.end())
			{
				keyValueMutimap.reset(new KeyValueMutimap());
				configMap_.insert(std::make_pair(sName, keyValueMutimap));
			}
			else 
			{
				keyValueMutimap = it->second;
			}
			continue;
		}
		ret = parseValue(data, key, value);
		if (ret == -2)
		{
			LOG_ERROR << "解析错误, Line: " << line << "," << data;
			fclose(fp);
			return false;
		}
		if (ret < 0)
		{
			continue;
		}
		if (!keyValueMutimap)
		{
			LOG_ERROR << "没在配置section, Line: " << line << "," << data;
			fclose(fp);
			return false;
		}
		keyValueMutimap->insert(std::make_pair(key, value));
	}
	fclose(fp);
	return true;
}

bool Config::getString(const std::string& section, const std::string& key, std::string* value) const
{
	SectionMap::const_iterator it = configMap_.find(section);
	if (it == configMap_.end())
	{
		return false;
	}

	KeyValueMutimap::const_iterator it1 = it->second->find(key);
	if (it1 == it->second->end())
	{
		return false;
	}

	*value = it1->second;
	return true;
}

bool Config::getStringList(const std::string& section, const std::string& key, std::vector<std::string>* values) const
{
	SectionMap::const_iterator it = configMap_.find(section);
	if (it == configMap_.end())
	{
		return false;
	}
	
	std::pair <KeyValueMutimap::const_iterator, KeyValueMutimap::const_iterator> ret;
	ret = it->second->equal_range(key);
	if (ret.first == it->second->end())
	{
		return false;
	}

	for (KeyValueMutimap::const_iterator it = ret.first; it != ret.second; ++it)
	{
		values->push_back(it->second);
	}
	return true;
}

bool Config::getInt32(const std::string& section, const std::string& key, int32_t* value) const
{
	std::string strVal;
	if (!getString(section, key, &strVal))
	{
		return false;
	}
	
	*value = base::StringUtil::strToInt32(strVal);
	return true;
}

bool Config::getInt32List(const std::string& section, const std::string& key, std::vector<int32_t>* values) const
{
	std::vector<std::string> strVals;
	if (!getStringList(section, key, &strVals))
	{
		return false;
	}
	
	for (size_t i = 0; i < strVals.size(); i++)
	{
		values->push_back(base::StringUtil::strToInt32(strVals[i]));
	}
	return true;
}

void Config::getSectionNames(std::vector<std::string>* sections) const
{
	SectionMap::const_iterator it;
	for (it = configMap_.begin(); it != configMap_.end(); ++it)
	{
		sections->push_back(it->first);
	}
}

bool Config::getSectionKeys(const std::string& section, std::vector<std::string>* keys) const
{
	SectionMap::const_iterator it = configMap_.find(section);
	if (it == configMap_.end())
	{
		return false;
	}

	KeyValueMutimap::const_iterator it1;
	for (it1 = it->second->begin(); it1 != it->second->end(); ++it1)
	{
		keys->push_back(it1->first);
	}
	return true;
}

} // namespace base

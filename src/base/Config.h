#ifndef BASE_CONFIG_H
#define BASE_CONFIG_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <vector>

#include <stdint.h>

namespace base
{

class Config : boost::noncopyable
{
public:
	bool load(const std::string& filename);
	bool getString(const std::string& section, const std::string& key, std::string* value) const;
	/// 取section下，相同key的所有值
	bool getStringList(const std::string& section, const std::string& key, std::vector<std::string>* values) const;
	bool getInt32(const std::string& section, const std::string& key, int32_t* value) const;
    bool getInt64(const std::string& section, const std::string& key, int64_t* value) const;
	/// 取section下，相同key的所有值
	bool getInt32List(const std::string& section, const std::string& key, std::vector<int32_t>* values) const;
    bool getInt64List(const std::string& section, const std::string& key, std::vector<int32_t>* values) const;
	/// 取所有section的名字
	void getSectionNames(std::vector<std::string>* sections) const;
	/// 取section下所有的key
	bool getSectionKeys(const std::string& section, std::vector<std::string>* keys) const;

private:
	int parseValue(char* str, char* key, char* val);
	char* isSectionName(char* str);

	typedef std::multimap<std::string, std::string> KeyValueMutimap;
	typedef boost::shared_ptr<KeyValueMutimap> KeyValueMutimapPtr;
	typedef std::map<std::string, KeyValueMutimapPtr> SectionMap;
	SectionMap configMap_;
};

} // namespace base

#endif // BASE_CONFIG_H

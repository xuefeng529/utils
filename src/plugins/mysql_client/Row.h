#ifndef PLUGIN_MYSQL_CLIENT_ROW_H
#define PLUGIN_MYSQL_CLIENT_ROW_H

#include <mysql.h>

#include <string>
#include <vector>
#include <map>
#include <sstream>

#include <stdint.h>

namespace plugin
{
namespace mysql
{

class Row
{
public:
	typedef std::map<std::string, uint32_t> FieldNameIndexMap;

	Row();
	Row(MYSQL_ROW mysqlRow, const FieldNameIndexMap* nameIndexMap);

	size_t fieldsCount() const
	{  return fieldsCount_; }

	const std::string& toString(uint32_t index) const;
	const std::string& toString(const std::string& name) const;

	int32_t toInt32(uint32_t index) const;
	int32_t toInt32(const std::string& name) const;
	uint32_t toUInt32(uint32_t index) const;
	uint32_t toUInt32(const std::string& name) const;

	int64_t toInt64(uint32_t index) const;
	int64_t toInt64(const std::string& name) const;
	uint64_t toUInt64(uint32_t index) const;
	uint64_t toUInt64(const std::string& name) const;

	float toFloat(uint32_t index) const;
	float toFloat(const std::string& name) const;

	double toDouble(uint32_t index) const;
	double toDouble(const std::string& name) const;

	void swap(Row& other);

private:
	const std::string& value(uint32_t index) const;
	const std::string& value(const std::string& name) const;

	MYSQL_ROW mysqlRow_;
	size_t fieldsCount_;
	const FieldNameIndexMap* nameIndexMap_;
	std::vector<std::string> row_;
};

} // namespace mysql
} // namespace plugin

#endif // PLUGIN_MYSQL_CLIENT_ROW_H

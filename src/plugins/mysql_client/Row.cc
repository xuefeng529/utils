#include "plugins/mysql_client/Row.h"
#include "base/StringUtil.h"

#include <assert.h>

namespace plugin
{
namespace mysql
{

Row::Row()
	: mysqlRow_(NULL),
	  fieldsCount_(0),
	  nameIndexMap_(NULL)
{
}

Row::Row(MYSQL_ROW mysqlRow, const FieldNameIndexMap* nameIndexMap)
	: mysqlRow_(mysqlRow),
	  fieldsCount_(nameIndexMap->size()),
	  nameIndexMap_(nameIndexMap)
{
	row_.reserve(fieldsCount_);
	for (size_t i = 0; i < fieldsCount_; ++i)
	{
		row_.push_back(mysqlRow_[i] != NULL ? mysqlRow_[i] : "");
	}
}

void Row::swap(Row& other)
{
	std::swap(mysqlRow_, other.mysqlRow_);
	std::swap(fieldsCount_, other.fieldsCount_);
	std::swap(nameIndexMap_, other.nameIndexMap_);
	row_.swap(other.row_);
}

const std::string& Row::value(uint32_t index) const
{
	assert(index < fieldsCount_);
	return row_[index];
}

const std::string& Row::value(const std::string& name) const
{
	FieldNameIndexMap::const_iterator it = nameIndexMap_->find(name);
	assert(it != nameIndexMap_->end());
	return value(it->second);
}

const std::string& Row::toString(uint32_t index) const
{
	return value(index);
}

const std::string& Row::toString(const std::string& name) const
{
	return value(name);
}

int32_t Row::toInt32(uint32_t index) const
{
	return base::StringUtil::strToInt32(value(index));
}

int32_t Row::toInt32(const std::string& name) const
{
	return base::StringUtil::strToInt32(value(name));
}

uint32_t Row::toUInt32(uint32_t index) const
{
	return base::StringUtil::strToUInt32(value(index));
}

uint32_t Row::toUInt32(const std::string& name) const
{
	return base::StringUtil::strToUInt32(value(name));
}

int64_t Row::toInt64(uint32_t index) const
{
	return base::StringUtil::strToInt64(value(index));
}

int64_t Row::toInt64(const std::string& name) const
{
	return base::StringUtil::strToInt64(value(name));
}

uint64_t Row::toUInt64(uint32_t index) const
{
	return base::StringUtil::strToUInt64(value(index));
}

uint64_t Row::toUInt64(const std::string& name) const
{
	return base::StringUtil::strToUInt64(value(name));
}

float Row::toFloat(uint32_t index) const
{
	return base::StringUtil::strToFloat(value(index));
}

float Row::toFloat(const std::string& name) const
{
	return base::StringUtil::strToFloat(value(name));
}

double Row::toDouble(uint32_t index) const
{
	return base::StringUtil::strToDouble(value(index));
}

double Row::toDouble(const std::string& name) const
{
	return base::StringUtil::strToDouble(value(name));
}

} // namespace mysql
} // namespace plugin

#include "plugins/redis/Client.h"
#include "base/Logging.h"
#include "net/Buffer.h"
#include "plugins/redis/Client.h"

#include <boost/scoped_ptr.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

boost::scoped_ptr<redis::Client> theCli;
const char kListName[] = "test_binary_list";

struct Entry
{
    std::string userId;
    std::string stockCode;
    float value;
    uint8_t index;

    void serialize(net::Buffer* buffer) const
    {
        buffer->appendInt32WithOriginalEndian(static_cast<int32_t>(userId.size()));
        buffer->append(userId);
        buffer->appendInt32WithOriginalEndian(static_cast<int32_t>(stockCode.size()));
        buffer->append(stockCode);
        buffer->append(&value, sizeof(value));
        buffer->appendInt8(index);
    }

    void unserialize(net::Buffer* buffer)
    {
        int32_t len = buffer->readInt32WithOriginalEndian();
        buffer->retrieveAsString(len, &userId);
        len = buffer->readInt32WithOriginalEndian();
        buffer->retrieveAsString(len, &stockCode);
        buffer->retrieveAsBytes(reinterpret_cast<char*>(&value), sizeof(value));
        index = buffer->readInt8();
    }
};

void save()
{
    LOG_INFO << "begin to save";
    for (int i = 1; i <= 10; i++)
    {
        std::vector<Entry> entries;
        for (int j = 1; j <= 10; j++)
        {
            char userId[64];
            snprintf(userId, sizeof(userId), "user_%03d%08d", i, j);
            char stockCode[64];
            snprintf(stockCode, sizeof(stockCode), "stock_%03d%08d", i, j);
            float value = 3.1415f;
            LOG_INFO << "value: " << value;
            Entry entry = { userId, stockCode, value, static_cast<uint8_t>(j) };
            entries.push_back(entry);
        }

        net::Buffer buffer;
        buffer.appendInt32WithOriginalEndian(static_cast<int32_t>(entries.size()));
        for (size_t i = 0; i < entries.size(); ++i)
        {
            entries[i].serialize(&buffer);
        }

        std::string val;
        buffer.retrieveAllAsString(&val);
        theCli->qpush(kListName, val);
    }

    LOG_INFO << "saving completed";
}

void load()
{
    LOG_INFO << "begin to load";
    const size_t interval = 999;
    std::vector<std::string> all;
    int64_t start = 0;
    int64_t stop = start + interval;
    while (true)
    {
        std::vector<std::string> tmp;
        theCli->qrange(kListName, start, stop, &tmp);
        all.insert(all.end(), tmp.begin(), tmp.end());
        if (tmp.size() < interval)
        {
            break;
        }

        start = stop + 1;
        stop = start + interval;
    }

    LOG_INFO << "list size: " << all.size();
    for (size_t i = 0; i < all.size(); i++)
    {
        net::Buffer buffer;   
        buffer.append(all[i]);
        size_t numEntries = buffer.readInt32WithOriginalEndian();
        for (size_t j = 0; j < numEntries; j++)
        {
            Entry entry;
            entry.unserialize(&buffer);
            LOG_INFO << "entry: " << entry.userId << " " << entry.stockCode << " "
                << entry.value  << " " << entry.index;
        }
    }

    LOG_INFO << "loading completed";
}

int main(int argc, char* argv[])
{
    theCli.reset(new redis::Client());
    theCli->connect(argv[1], atoi(argv[2]), argv[3]);
    save();
    load();
    theCli->del(kListName);
}

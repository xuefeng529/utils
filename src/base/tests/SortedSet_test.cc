#include "base/SortedSet.h"

#include <boost/functional/hash.hpp>
#include <iostream>
#include <vector>

#include <assert.h>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

struct Entry
{
    std::string key;
    float value;

    size_t operator()(const Entry& other) const
    {
        return boost::hash<std::string>()(other.key);
    }

    bool operator==(const Entry& right) const
    {
        return key == right.key;
    }

    bool operator<(const Entry& right) const
    {
        if (key == right.key)
        {
            return false;
        }
        else
        {
            if (value != right.value)
            {
                return value < right.value;
            }
            else
            {
                return key < right.key;
            }
        }
    }
};

int main(int argc, char* argv[])
{
    size_t count = static_cast<size_t>(atoi(argv[1]));
    
    std::vector<std::string> allStr;
    for (size_t i = 0; i < count; ++i)
    {
        char key[128];
        snprintf(key, sizeof(key), "key_%"PRIu64, i);
        allStr.push_back(key);
    }
    std::cout << "test insert string to std::set\n";
    std::set<std::string> strset;
    time_t begin = time(NULL);
    for (size_t i = 0; i < allStr.size(); i++)
    {
        std::pair<std::set<std::string>::const_iterator, bool>
            ret = strset.insert(allStr[i]);
        assert(ret.second);
    }
    time_t end = time(NULL);
    std::cout << "diff time: " << end - begin << "(secs)" << std::endl;

    std::vector<Entry> allVec;
    std::vector<Entry> findVec;
    std::vector<Entry> remainVec;
    std::vector<Entry> eraseVec;
    base::SortedSet<Entry, Entry> entrySet;
    for (size_t i = 0; i < count; ++i)
    {        
        char key[128];
        snprintf(key, sizeof(key), "key_%"PRIu64, i);
        Entry entry = { key, 8.8f * i };
        allVec.push_back(entry);
        findVec.push_back(entry);
        if (i % 2)
        {
            eraseVec.push_back(entry);
        }
        else
        {
            remainVec.push_back(entry);
        }
    }

    std::cout << "test insert\n";
    begin = time(NULL);
    for (size_t i = 0; i < allVec.size(); i++)
    {
        assert(entrySet.insert(allVec[i]));
    }
    end = time(NULL);
    std::cout << "diff time: " << end - begin << "(secs)" << std::endl;

    std::cout << "test reinsert\n";
    for (size_t i = 0; i < remainVec.size(); i++)
    {
        assert(!entrySet.insert(remainVec[i]));
    }

    std::cout << "test traversing\n";
    size_t n = 0;
    base::SortedSet<Entry, Entry>::const_iterator it = entrySet.begin();
    for (; it != entrySet.end(); ++it)
    {
        //std::cout << it->key << ", " << it->value << std::endl;
        n++;
    }
    assert(count == n);
    assert(n == entrySet.size());

    std::cout << "test find\n";
    for (size_t i = 0; i < findVec.size(); i++)
    {
        it = entrySet.find(findVec[i]);
        assert(it != entrySet.end());
    }

    std::cout << "test erase\n";
    for (size_t i = 0; i < eraseVec.size(); i++)
    {
        size_t n = entrySet.erase(eraseVec[i]);
        assert(n == 1);
    }

    std::cout << "test traversing after erase\n";
    n = 0;
    it = entrySet.begin();
    for (; it != entrySet.end(); ++it)
    {
        //std::cout << it->key << ", " << it->value << std::endl;
        n++;
    }
    assert(n == remainVec.size());

    std::cout << "test find after erase\n";
    for (size_t i = 0; i < eraseVec.size(); i++)
    {
        it = entrySet.find(eraseVec[i]);
        assert(it == entrySet.end());
    }

    std::cout << "test remain\n";
    for (size_t i = 0; i < remainVec.size(); i++)
    {
        it = entrySet.find(remainVec[i]);
        assert(it != entrySet.end());
    }

    return 0;
}

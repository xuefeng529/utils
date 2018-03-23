#include "plugins/zk/util.h"
#include "base/Logging.h"

namespace plugins
{
namespace zk
{

bool isChild(const std::string& child, const std::string& parent)
{
    size_t childLen = child.size();
    size_t parentLen = parent.size();
    if (child[childLen - 1] == '/') 
    {
        childLen--;
    }

    if (parent[parentLen - 1] == '/') 
    {
        parentLen--;
    }

    if (childLen <= parentLen || 0 != strncmp(parent.data(), child.data(), parentLen)
        || child[parentLen] != '/' || child[parentLen + 1] == '\0')
    {
        return false;
    }

    const char* slash = strchr(child.data() + parentLen + 1, '/');
    if (slash == NULL || slash == child.data() + childLen)
    {
        return false;
    }

    return true;
}

bool getParentPath(const std::string& path, std::string* parent) 
{
    if (path[0] != '/') 
    {
        return false;
    }

    size_t lastSlashPos = path.find_last_of('/');
    if (lastSlashPos > 0)
    {
        parent->assign(path, 0, lastSlashPos);
    }
    else
    {
        parent->assign("/");
    }
    return true;
}

const char* getNodeName(const std::string& path)
{
    if (path[0] != '/') 
    {
        return NULL;
    }
    const char* lastSlash = rindex(path.c_str(), '/');
    return lastSlash + 1;
}

int32_t getSequenceNo(const std::string& name) 
{
    size_t nameLen = name.size();
    if (nameLen < 10)
    {
        LOG_ERROR << "name [" << name << "] too short";
        return -1;
    }

    const char* seqStr = name.data() + nameLen - 10;
    while (*seqStr == '0')
    {
        seqStr++;
    }

    int32_t seqNo;
    char* seqEnd;
    if (*seqStr != '\0') 
    {
        seqNo = strtol(seqStr, &seqEnd, 10);
        if (*seqEnd == '\0' && seqNo > 0)
        {
            return seqNo;
        }
        else 
        {
            LOG_ERROR << "name [" << name << "] not end in 10 digit";
            return -1;
        }
    }
    else 
    {
        return 0;
    }
}

bool isValidPath(const std::string& path)
{
    if (path.empty() || path[0] != '/'
        || (path.size() > 1 && *path.rbegin() == '/'))
    {
        return false;
    }
    return true;
}

} // namespace zk
} // namespace plugins

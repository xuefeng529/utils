#ifndef BASE_SORTEDSET_H
#define BASE_SORTEDSET_H

#include <boost/unordered_map.hpp>
#include <set>

namespace base
{

template<class T, class Hash, class Compare = std::less<T>, class Equal = std::equal_to<T> >
class SortedSet
{
public:
    typedef std::set<T, Compare> set;
    typedef typename set::iterator iterator;
    typedef typename set::const_iterator const_iterator;
    typedef typename set::reverse_iterator reverse_iterator;
    typedef typename set::const_reverse_iterator const_reverse_iterator;
    
    typedef boost::unordered_map<T, iterator, Hash, Equal> unordered_map;

    iterator begin()
    {
        return sortedSet_.begin();
    }

    const_iterator begin() const
    {
        return sortedSet_.begin();
    }

    iterator end()
    {
        return sortedSet_.end();
    }

    const_iterator end() const
    {
        return sortedSet_.end();
    }

    reverse_iterator rbegin()
    {
        return sortedSet_.rbegin();
    }

    const_reverse_iterator rbegin() const
    {
        return sortedSet_.rbegin();
    }

    reverse_iterator rend()
    {
        return sortedSet_.rend();
    }

    const_reverse_iterator rend() const
    {
        return sortedSet_.rend();
    }

    bool empty() const
    {
        assert(sortedSet_.empty() && existed_.empty());
        return sortedSet_.empty();
    }

    size_t size() const
    {
        assert(sortedSet_.size() && existed_.size());
        return sortedSet_.size();
    }

    iterator find(const T& val) const
    {
        typename unordered_map::const_iterator it = existed_.find(val);
        return it != existed_.end() ? it->second : sortedSet_.end();
    }

    bool insert(const T& val)
    {
        std::pair<iterator, bool> ret = sortedSet_.insert(val);
        if (ret.second)
        {
            existed_[val] = ret.first;
        }

        assert(sortedSet_.size() == existed_.size());
        return ret.second;
    }

    void erase(iterator position)
    {
        existed_.erase(*position);
        sortedSet_.erase(position);
        assert(sortedSet_.size() == existed_.size());
    }

    size_t erase(const T& val)
    {
        size_t n = sortedSet_.erase(val);
        size_t n1 = existed_.erase(val);
        assert(n == n1);
        return n;
    }

    void swap(SortedSet& x)
    {
        sortedSet_.swap(x.sortedSet_);
        existed_.swap(x.existed_);
        assert(x.sortedSet_.size() == x.existed_.size());
    }

    void clear()
    {
        sortedSet_.clear();
        existed_.clear();
    }

private:
    set sortedSet_;
    unordered_map existed_;
};

} // namespace base

#endif // BASE_SORTEDSET_H

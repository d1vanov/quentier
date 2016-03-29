#ifndef __LIB_QUTE_NOTE__UTILITY__LRU_CACHE_HPP
#define __LIB_QUTE_NOTE__UTILITY__LRU_CACHE_HPP

#include <qute_note/utility/Qt4Helper.h>
#include <list>
#include <cstddef>
#include <QHash>

namespace qute_note {

template<class Key, class Value, class Allocator = std::allocator<std::pair<const Key, Value> > >
class LRUCache
{
public:
    LRUCache(const size_t maxSize = 100) :
        m_maxSize(maxSize)
    {}

    typedef Key key_type;
    typedef Value mapped_type;
    typedef Allocator allocator_type;
    typedef std::pair<key_type, mapped_type> value_type;
    typedef std::list<value_type, allocator_type> container_type;
    typedef typename container_type::size_type size_type;
    typedef typename container_type::difference_type difference_type;
    typedef typename container_type::iterator iterator;
    typedef typename container_type::const_iterator const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

#if __cplusplus < 201103L
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
#else
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef typename std::allocator_traits<allocator_type>::pointer pointer;
    typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;
#endif

    iterator begin() { return m_container.begin(); }
    const_iterator begin() const { return m_container.begin(); }
    
    reverse_iterator rbegin() { return m_container.rbegin(); }
    const_reverse_iterator rbegin() const { return m_container.rbegin(); }

    iterator end() { return m_container.end(); }
    const_iterator end() const { return m_container.end(); }

    reverse_iterator rend() { return m_container.rend(); }
    const_reverse_iterator rend() const { return m_container.rend(); }

    bool empty() const { return m_container.empty(); }
    size_t size() const { return m_currentSize; }
    size_t max_size() const { return m_maxSize; }

    void clear() { m_container.clear(); m_mapper.clear(); }

    void put(const key_type & key, const mapped_type & value)
    {
        auto mapperIt = m_mapper.find(key);
        if (mapperIt != m_mapper.end()) {
            auto it = mapperIt.value();
            Q_UNUSED(m_container.erase(it))
            Q_UNUSED(m_mapper.erase(mapperIt))
        }

        m_container.push_front(value_type(key, value));
        m_mapper[key] = m_container.begin();

        if (m_container.size() > m_maxSize) {
            auto lastIt = m_container.end();
            --lastIt;

            const key_type & lastElementKey = lastIt->first;
            Q_UNUSED(m_mapper.remove(lastElementKey))
            Q_UNUSED(m_container.erase(lastIt))
        }
    }

    const mapped_type * get(const key_type & key) const
    {
        auto mapperIt = m_mapper.find(key);
        if (mapperIt == m_mapper.end()) {
            return Q_NULLPTR;
        }

        m_container.splice(m_container.begin(), m_container, mapperIt.value());
        return &mapperIt.value()->second;
    }

    bool exists(const key_type & key)
    {
        auto mapperIt = m_mapper.find(key);
        return (mapperIt != m_mapper.end());
    }

    bool remove(const key_type & key)
    {
        auto mapperIt = m_mapper.find(key);
        if (mapperIt == m_mapper.end()) {
            return false;
        }

        auto it = mapperIt.value();
        Q_UNUSED(m_container.erase(it))
        Q_UNUSED(m_mapper.erase(mapperIt))
        return true;
    }

private:
    mutable container_type      m_container;
    size_t                      m_currentSize;
    size_t                      m_maxSize;

    QHash<QString, iterator>    m_mapper;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__LRU_CACHE_HPP

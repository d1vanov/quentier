#ifndef __LIB_QUTE_NOTE__UTILITY__LIMITED_STACK_H
#define __LIB_QUTE_NOTE__UTILITY__LIMITED_STACK_H

#include <QStack>

namespace qute_note {

/**
 * @brief The LimitedStack template class implements a stack which may have a limitation for its size;
 * when the size becomes too much according to the limit, the bottom element of the stack gets erased from it.
 * Only limits greater than zero are considered.
 */
template <class T>
class LimitedStack: public QStack<T>
{
public:
    LimitedStack() : m_limit(-1) {}
    LimitedStack(const LimitedStack<T> & other) : QStack<T>(other), m_limit(other.m_limit) {}
    LimitedStack(LimitedStack<T> && other) : QStack<T>(std::move(other)), m_limit(std::move(other.m_limit)) {}

    LimitedStack<T> & operator=(const LimitedStack<T> & other)
    {
        if (this != &other) {
            QStack<T>::operator=(other);
            m_limit = other.m_limit;
        }

        return *this;
    }

    LimitedStack<T> & operator=(LimitedStack<T> && other)
    {
        if (this != &other) {
            QStack<T>::operator=(std::move(other));
            m_limit = std::move(other.m_limit);
        }

        return *this;
    }

    ~LimitedStack() {}

    void swap(const LimitedStack<T> & other)
    {
        int limit = other.m_limit;
        other.m_limit = m_limit;
        m_limit = limit;

        QStack<T>::swap(other);
    }

    int limit() const { return m_limit; }
    void setLimit(const int limit) { m_limit = limit; }

    void push(const T & t)
    {
        if ((m_limit > 0) && (QVector<T>::size() == m_limit - 1)) {
            Q_UNUSED(QVector<T>::erase(QVector<T>::begin()));
        }

        QStack<T>::push(t);
    }

private:
    int  m_limit;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__LIMITED_STACK_H

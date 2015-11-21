#ifndef __LIB_QUTE_NOTE__UTILITY__LIMITED_STACK_H
#define __LIB_QUTE_NOTE__UTILITY__LIMITED_STACK_H

#include <qute_note/utility/Qt4Helper.h>
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
    LimitedStack(void (*deleter)(T&) = Q_NULLPTR) : m_limit(-1), m_deleter(deleter) {}
    LimitedStack(const LimitedStack<T> & other) : QStack<T>(other), m_limit(other.m_limit), m_deleter(other.m_deleter) {}
    LimitedStack(LimitedStack<T> && other) : QStack<T>(std::move(other)), m_limit(std::move(other.m_limit)), m_deleter(std::move(other.m_deleter)) {}

    LimitedStack<T> & operator=(const LimitedStack<T> & other)
    {
        if (this != &other) {
            QStack<T>::operator=(other);
            m_limit = other.m_limit;
            m_deleter = other.m_deleter;
        }

        return *this;
    }

    LimitedStack<T> & operator=(LimitedStack<T> && other)
    {
        if (this != &other) {
            QStack<T>::operator=(std::move(other));
            m_limit = std::move(other.m_limit);
            m_deleter = std::move(other.m_deleter);
        }

        return *this;
    }

    ~LimitedStack()
    {
        if (m_deleter)
        {
            while (!QStack<T>::isEmpty()) {
                T t = QStack<T>::pop();
                (*m_deleter)(t);
            }
        }
    }

    void swap(const LimitedStack<T> & other)
    {
        int limit = other.m_limit;
        other.m_limit = m_limit;
        m_limit = limit;

        void (*deleter)(T &) = other.m_deleter;
        other.m_deleter = m_deleter;
        m_deleter = deleter;

        QStack<T>::swap(other);
    }

    int limit() const { return m_limit; }
    void setLimit(const int limit) { m_limit = limit; }

    void push(const T & t)
    {
        if ((m_limit > 0) && (QVector<T>::size() == m_limit - 1)) {
            if (m_deleter) {
                (*m_deleter)(*QVector<T>::begin());
            }
            Q_UNUSED(QVector<T>::erase(QVector<T>::begin()));
        }

        QStack<T>::push(t);
    }

private:
    int  m_limit;
    void (*m_deleter)(T &);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__LIMITED_STACK_H

#ifndef __QUTE_NOTE__TOOLS__SINGLETON_H
#define __QUTE_NOTE__TOOLS__SINGLETON_H

/**
 * @brief Primitive but effective and thread-safe since C++11
 * implementation of singleton pattern
 *
 * Singleton class is a primitive template wrapper over usual
 * Meyers' singleton, thread-safe per C++11 standard.
 * The actual singleton has type T which is a template argument of this class.
 * Class T needs to have public standard constructor without arguments.
 */
template <class T>
class Singleton
{
public:
    static T & Instance()
    {
        static T instance;
        return instance;
    }

private:
    Singleton() = delete;
    ~Singleton() = delete;
    Singleton(const Singleton & other) = delete;
    Singleton & operator=(const Singleton & other) = delete;
};

#endif // __QUTE_NOTE__TOOLS__SINGLETON_H

#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__JS_RESULT_CALLBACK_FUNCTOR_HPP
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__JS_RESULT_CALLBACK_FUNCTOR_HPP

#include <QVariant>

namespace qute_note {

template <class T>
class JsResultCallbackFunctor
{
public:
    typedef void (T::*Method)(const QVariant &);

    JsResultCallbackFunctor(T & object, Method method) :
        m_object(object),
        m_method(method)
    {}

    void operator()(const QVariant & data) { (m_object.*m_method)(data); }

private:
    T &         m_object;
    Method      m_method;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__JS_RESULT_CALLBACK_FUNCTOR_HPP

#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_JS_RESULT_CALLBACK_FUNCTOR_HPP
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_JS_RESULT_CALLBACK_FUNCTOR_HPP

#include <QVariant>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_JS_RESULT_CALLBACK_FUNCTOR_HPP

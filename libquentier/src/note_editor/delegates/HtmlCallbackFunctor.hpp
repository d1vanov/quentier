#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_HTML_CALLBACK_FUNCTOR_HPP
#define LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_HTML_CALLBACK_FUNCTOR_HPP

#include <QVariant>

namespace qute_note {

template <class T>
class HtmlCallbackFunctor
{
public:
    typedef void (T::*Method)(const QString &);

    HtmlCallbackFunctor(T & object, Method method) :
        m_object(object),
        m_method(method)
    {}

    void operator()(const QString & html) { (m_object.*m_method)(html); }

private:
    T &         m_object;
    Method      m_method;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_HTML_CALLBACK_FUNCTOR_HPP

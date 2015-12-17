#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__HTML_CALLBACK_FUNCTOR_HPP
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__HTML_CALLBACK_FUNCTOR_HPP

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

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__HTML_CALLBACK_FUNCTOR_HPP

/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

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

/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef LIB_QUENTIER_TYPES_ERROR_STRING_H
#define LIB_QUENTIER_TYPES_ERROR_STRING_H

#include <quentier/utility/Printable.h>
#include <QSharedDataPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ErrorStringData)

/**
 * @brief The ErrorString class encapsulates two strings which are meant to contain
 * translatable (base) and non-translatable (details) parts of the error description
 */
class QUENTIER_EXPORT ErrorString: public Printable
{
public:
    explicit ErrorString(const QString & base, const QString & details = QString());
    ErrorString(const ErrorString & other);
    ErrorString & operator=(const ErrorString & other);
    virtual ~ErrorString();

    const QString & base() const;
    QString & base();

    const QString & details() const;
    QString & details();

    bool isEmpty() const;
    void clear();

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<ErrorStringData> d;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_ERROR_STRING_H

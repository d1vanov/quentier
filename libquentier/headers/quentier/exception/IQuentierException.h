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

#ifndef LIB_QUENTIER_EXCEPTION_I_QUENTIER_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_I_QUENTIER_EXCEPTION_H

#include <quentier/utility/Printable.h>
#include <quentier/utility/QNLocalizedString.h>
#include <exception>

namespace quentier {

class QUENTIER_EXPORT IQuentierException: public Printable,
                                          public std::exception
{
public:
    explicit IQuentierException(const QNLocalizedString & message);

#ifdef _MSC_VER
    virtual ~IQuentierException();
#elif __cplusplus >= 201103L
    virtual ~IQuentierException() noexcept;
#else
    virtual ~IQuentierException() throw();
#endif

    QString localizedErrorMessage() const;
    QString nonLocalizedErrorMessage() const;

#ifdef _MSC_VER
    virtual const char * what() const Q_DECL_OVERRIDE;
#elif __cplusplus >= 201103L
    virtual const char * what() const noexcept Q_DECL_OVERRIDE;
#else
    virtual const char * what() const Q_DECL_OVERRIDE throw();
#endif

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

protected:
    IQuentierException(const IQuentierException & other);
    IQuentierException & operator =(const IQuentierException & other);

    virtual const QString exceptionDisplayName() const = 0;

private:
    IQuentierException() Q_DECL_DELETE;

    QNLocalizedString   m_message;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_I_QUENTIER_EXCEPTION_H


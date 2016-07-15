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

#ifndef LIB_QUENTIER_UTILITY_QUENTIER_CHECK_PTR_H
#define LIB_QUENTIER_UTILITY_QUENTIER_CHECK_PTR_H

#include <quentier/exception/NullPtrException.h>

#ifndef QUENTIER_CHECK_PTR
#define QUENTIER_CHECK_PTR(pointer, ...) \
{ \
    if (Q_UNLIKELY(!pointer)) \
    { \
        using quentier::NullPtrException; \
        QNLocalizedString quentier_null_ptr_error = QT_TR_NOOP("Detected the unintended null pointer at"); \
        quentier_null_ptr_error += " "; \
        quentier_null_ptr_error += __FILE__; \
        quentier_null_ptr_error += " ("; \
        quentier_null_ptr_error += QString::number(__LINE__); \
        quentier_null_ptr_error += ") "; \
        QNLocalizedString message = "" #__VA_ARGS__ ""; \
        quentier_null_ptr_error += message; \
        throw NullPtrException(quentier_null_ptr_error); \
    } \
}
#endif

#endif // LIB_QUENTIER_UTILITY_QUENTIER_CHECK_PTR_H

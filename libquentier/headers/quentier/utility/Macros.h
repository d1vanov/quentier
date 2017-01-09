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

#ifndef LIB_QUENTIER_UTILITY_MACROS_H
#define LIB_QUENTIER_UTILITY_MACROS_H

#include <QtGlobal>
#include <QString>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

#ifndef Q_DECL_OVERRIDE
#ifdef CPP11_COMPLIANT
#define Q_DECL_OVERRIDE override
#else
#define Q_DECL_OVERRIDE
#endif
#endif // Q_DECL_OVERRIDE

#ifndef Q_DECL_FINAL
#ifdef CPP11_COMPLIANT
#define Q_DECL_FINAL final
#else
#define Q_DECL_FINAL
#endif
#endif // Q_DECL_FINAL

#ifndef Q_STATIC_ASSERT_X
#ifdef CPP11_COMPLIANT
#define Q_STATIC_ASSERT_X(x1,x2) static_assert(x1, x2)
#else
#define Q_STATIC_ASSERT_X(x1,x2)
#endif
#endif // Q_STATIC_ASSERT_X

#ifndef QStringLiteral
#define QStringLiteral(x) QString::fromUtf8(x, sizeof(x) - 1)
#endif

#ifndef Q_NULLPTR
#ifdef CPP11_COMPLIANT
#define Q_NULLPTR nullptr
#else
#define Q_NULLPTR NULL
#endif
#endif

#endif // QT_VERSION

#ifdef QNSIGNAL
#undef QNSIGNAL
#endif

#ifdef QNSLOT
#undef QNSLOT
#endif

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || (defined(_MSC_VER) && (_MSC_VER <= 1600))
#define QNSIGNAL(className, methodName, ...) SIGNAL(methodName(__VA_ARGS__))
#define QNSLOT(className, methodName, ...) SLOT(methodName(__VA_ARGS__))
#else
#define QNSIGNAL(className, methodName, ...) &className::methodName
#define QNSLOT(className, methodName, ...) &className::methodName
#endif

#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#ifdef QStringLiteral
#undef QStringLiteral
#define QStringLiteral(x) QString::fromUtf8(x, sizeof(x) - 1)
#endif
#endif

#ifndef Q_DECL_EQ_DELETE
#ifdef CPP11_COMPLIANT
#define Q_DECL_EQ_DELETE = delete
#else
#define Q_DECL_EQ_DELETE
#endif
#endif // Q_DECL_EQ_DELETE

#endif // LIB_QUENTIER_UTILITY_MACROS_H

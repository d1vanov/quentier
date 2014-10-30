#ifndef QT4HELPER_H
#define QT4HELPER_H

#include <QtGlobal>

#if QT_VERSION < 0x050000

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
#define QStringLiteral(x) QString(QLatin1String(x))
#endif

#endif // QT_VERSION

#ifndef Q_DECL_DELETE
#ifdef CPP11_COMPLIANT
#define Q_DECL_DELETE = delete
#else
#define Q_DECL_DELETE
#endif
#endif // Q_DECL_DELETE

#endif // QT4HELPER_H

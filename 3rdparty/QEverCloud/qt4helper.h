#ifndef QT4HELPER_H
#define QT4HELPER_H

#if QT_VERSION < 0x050000
#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE override
#endif
#ifndef Q_STATIC_ASSERT_X
#define Q_STATIC_ASSERT_X(x1,x2)
#endif
#ifndef QStringLiteral
#define QStringLiteral(x) QString(QLatin1String(x))
#endif
#endif

#endif // QT4HELPER_H

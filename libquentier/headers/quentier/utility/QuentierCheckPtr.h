#ifndef LIB_QUENTIER_UTILITY_QUENTIER_CHECK_PTR_H
#define LIB_QUENTIER_UTILITY_QUENTIER_CHECK_PTR_H

#include <quentier/exception/NullPtrException.h>

#ifndef QUENTIER_CHECK_PTR
#define QUENTIER_CHECK_PTR(pointer, ...) \
{ \
    if (Q_UNLIKELY(!pointer)) \
    { \
        using quentier::NullPtrException; \
        QString quentier_null_ptr_error = "Found NULL pointer at "; \
        quentier_null_ptr_error += __FILE__; \
        quentier_null_ptr_error += " ("; \
        quentier_null_ptr_error += QString::number(__LINE__); \
        quentier_null_ptr_error += ") "; \
        QString message = "" #__VA_ARGS__ ""; \
        quentier_null_ptr_error += message; \
        throw NullPtrException(quentier_null_ptr_error); \
    } \
}
#endif

#endif // LIB_QUENTIER_UTILITY_QUENTIER_CHECK_PTR_H

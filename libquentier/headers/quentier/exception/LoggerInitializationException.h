#ifndef LIB_QUENTIER_EXCEPTION_LOGGER_INITIALIZATION_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_LOGGER_INITIALIZATION_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT LoggerInitializationException: public IQuentierException
{
public:
    explicit LoggerInitializationException(const QNLocalizedString & message);

protected:
    const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_LOGGER_INITIALIZATION_EXCEPTION_H

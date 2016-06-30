#ifndef LIB_QUENTIER_EXCEPTION_APPLICATION_SETTINGS_INITIALIZATION_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_APPLICATION_SETTINGS_INITIALIZATION_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT ApplicationSettingsInitializationException: public IQuentierException
{
public:
    explicit ApplicationSettingsInitializationException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_APPLICATION_SETTINGS_INITIALIZATION_EXCEPTION_H

#ifndef LIB_QUENTIER_UTILITY_APPLICATION_SETTINGS_H
#define LIB_QUENTIER_UTILITY_APPLICATION_SETTINGS_H

#include <quentier/utility/Printable.h>
#include <QSettings>

namespace quentier {

class QUENTIER_EXPORT ApplicationSettings: public Printable,
                                            public QSettings
{
public:
    ApplicationSettings();
    ~ApplicationSettings();

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(ApplicationSettings)
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_APPLICATION_SETTINGS_H

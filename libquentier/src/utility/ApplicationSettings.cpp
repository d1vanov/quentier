#include <quentier/utility/ApplicationSettings.h>
#include <quentier/exception/ApplicationSettingsInitializationException.h>
#include <QApplication>

namespace quentier {

ApplicationSettings::ApplicationSettings() :
    QSettings(QApplication::organizationName(), QApplication::applicationName())
{
    if (Q_UNLIKELY(QApplication::organizationName().isEmpty())) {
        throw ApplicationSettingsInitializationException("can't create ApplicationSettings instance: organization name is empty");
    }

    if (Q_UNLIKELY(QApplication::applicationName().isEmpty())) {
        throw ApplicationSettingsInitializationException("can't create ApplicationSettings instance: application name is empty");
    }
}

ApplicationSettings::~ApplicationSettings()
{}

QTextStream & ApplicationSettings::print(QTextStream & strm) const
{
    QStringList allStoredKeys = QSettings::allKeys();

    foreach(const QString & key, allStoredKeys) {
        QVariant value = QSettings::value(key);
        strm << "Key: " << key << "; Value: " << value.toString() << "\n;";
    }

    return strm;
}

} // namespace quentier

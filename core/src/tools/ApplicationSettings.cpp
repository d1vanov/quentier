#include "ApplicationSettings.h"

namespace qute_note {

ApplicationSettings & ApplicationSettings::instance(const QString & orgName, const QString & appName)
{
    static ApplicationSettings instance(orgName, appName);
    return instance;
}

ApplicationSettings::ApplicationSettings(const QString & orgName, const QString & appName) :
    m_settings(orgName, appName)
{}

ApplicationSettings::~ApplicationSettings()
{}

QTextStream & ApplicationSettings::Print(QTextStream & strm) const
{
    QStringList allKeys = m_settings.allKeys();

    foreach(const QString & key, allKeys)
    {
        QVariant value = m_settings.value(key);
        strm << "Key: " << key << "; Value: " << value.toString() << "\n;";
    }

    return strm;
}



} // namespace qute_note

#include <qute_note/utility/ApplicationSettings.h>

namespace qute_note {

ApplicationSettings & ApplicationSettings::instance(const QString & orgName, const QString & appName)
{
    static ApplicationSettings instance(orgName, appName);
    return instance;
}

QVariant ApplicationSettings::value(const QString & key, const QVariant & defaultValue) const
{
    return m_settings.value(key, defaultValue);
}

QVariant ApplicationSettings::value(const QString & key, const QString & keyGroup,
                                    const QVariant & defaultValue) const
{
    if (!keyGroup.isEmpty()) {
        return m_settings.value(keyGroup + "/" + key, defaultValue);
    }
    else {
        return m_settings.value(key, defaultValue);
    }
}

void ApplicationSettings::setValue(const QString & key, const QVariant & value, const QString & keyGroup)
{
    bool hasKeyGroup = !keyGroup.isEmpty();

    if (hasKeyGroup) {
        m_settings.beginGroup(keyGroup);
    }

    m_settings.setValue(key, value);

    if (hasKeyGroup) {
        m_settings.endGroup();
    }
}

int ApplicationSettings::beginReadArray(const QString & prefix)
{
    return m_settings.beginReadArray(prefix);
}

void ApplicationSettings::beginWriteArray(const QString & prefix, int size)
{
    m_settings.beginWriteArray(prefix, size);
}

void ApplicationSettings::setArrayIndex(int i)
{
    m_settings.setArrayIndex(i);
}

void ApplicationSettings::endArray()
{
    m_settings.endArray();
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

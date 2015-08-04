#ifndef __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H
#define __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H

#include <qute_note/utility/Printable.h>
#include <QSettings>

namespace qute_note {

class QUTE_NOTE_EXPORT ApplicationSettings : public Printable
{
public:
    static ApplicationSettings & instance();

    QVariant value(const QString & key, const QVariant & defaultValue = QVariant()) const;
    QVariant value(const QString & key, const QString & keyGroup, const QVariant & defaultValue = QVariant()) const;
    void setValue(const QString & key, const QVariant & value, const QString & keyGroup = QString());

    int beginReadArray(const QString & prefix);
    void beginWriteArray(const QString & prefix, int size = -1);
    void setArrayIndex(int i);
    void endArray();

    void beginGroup(const QString & keyGroup);
    void endGroup();

    QStringList childGroups() const;
    QStringList childKeys() const;

    bool isWritable() const;

private:
    ApplicationSettings() Q_DECL_DELETE;
    Q_DISABLE_COPY(ApplicationSettings);

    ApplicationSettings(const QString & orgName, const QString & appName);
    ~ApplicationSettings();

private:
    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSettings   m_settings;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H

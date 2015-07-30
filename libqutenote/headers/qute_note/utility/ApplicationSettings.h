#ifndef __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H
#define __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H

#include <qute_note/utility/Printable.h>
#include <QSettings>

namespace qute_note {

class QUTE_NOTE_EXPORT ApplicationSettings : public Printable
{
public:
    static ApplicationSettings & instance(const QString & orgName = "d1vanov",
                                          const QString & appName = "QuteNote");

    QVariant value(const QString & key, const QVariant & defaultValue = QVariant()) const;
    QVariant value(const QString & key, const QString & keyGroup, const QVariant & defaultValue = QVariant()) const;
    void setValue(const QString & key, const QVariant & value, const QString & keyGroup = QString());

    int beginReadArray(const QString & prefix);
    void beginWriteArray(const QString & prefix, int size = -1);
    void setArrayIndex(int i);
    void endArray();

private:
    ApplicationSettings() Q_DECL_DELETE;
    ApplicationSettings(const ApplicationSettings & other) Q_DECL_DELETE;
    ApplicationSettings(ApplicationSettings && other) Q_DECL_DELETE;
    ApplicationSettings & operator=(const ApplicationSettings & other) Q_DECL_DELETE;
    ApplicationSettings & operator=(ApplicationSettings && other) Q_DECL_DELETE;

    ApplicationSettings(const QString & orgName, const QString & appName);
    ~ApplicationSettings();

private:
    QTextStream & Print(QTextStream & strm) const;

private:

    QSettings   m_settings;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H

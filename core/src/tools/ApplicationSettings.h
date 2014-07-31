#ifndef __QUTE_NOTE__CORE__TOOLS__APPLICATION_SETTINGS_H
#define __QUTE_NOTE__CORE__TOOLS__APPLICATION_SETTINGS_H

#include "Printable.h"
#include <QSettings>

namespace qute_note {

class QUTE_NOTE_EXPORT ApplicationSettings : public Printable
{
public:
    static ApplicationSettings & instance(const QString & orgName = "d1vanov",
                                          const QString & appName = "QuteNote");

private:
    ApplicationSettings() = delete;
    ApplicationSettings(const ApplicationSettings & other) = delete;
    ApplicationSettings(ApplicationSettings && other) = delete;
    ApplicationSettings & operator=(const ApplicationSettings & other) = delete;
    ApplicationSettings & operator=(ApplicationSettings && other) = delete;

    ApplicationSettings(const QString & orgName, const QString & appName);
    ~ApplicationSettings();

private:
    QTextStream & Print(QTextStream & strm) const;

private:

    QSettings   m_settings;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__APPLICATION_SETTINGS_H

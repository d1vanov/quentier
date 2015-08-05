#ifndef __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H
#define __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H

#include <qute_note/utility/Printable.h>
#include <QSettings>

namespace qute_note {

class QUTE_NOTE_EXPORT ApplicationSettings: public Printable,
                                            public QSettings
{
public:
    ApplicationSettings();
    ~ApplicationSettings();

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(ApplicationSettings);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__APPLICATION_SETTINGS_H

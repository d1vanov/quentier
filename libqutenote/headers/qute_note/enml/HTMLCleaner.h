#ifndef LIB_QUTE_NOTE_ENML_HTML_CLEANER_H
#define LIB_QUTE_NOTE_ENML_HTML_CLEANER_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QString>

namespace qute_note {

class QUTE_NOTE_EXPORT HTMLCleaner
{
public:
    HTMLCleaner();
    virtual ~HTMLCleaner();

    bool htmlToXml(const QString & html, QString & output, QString & errorDescription);
    bool htmlToXhtml(const QString & html, QString & output, QString & errorDescription);
    bool cleanupHtml(QString & html, QString & errorDescription);

private:
    Q_DISABLE_COPY(HTMLCleaner)

private:
    class Impl;
    Impl * m_impl;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_ENML_HTML_CLEANER_H

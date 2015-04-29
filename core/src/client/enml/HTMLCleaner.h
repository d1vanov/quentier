#ifndef __QUTE_NOTE__CORE__TOOLS__HTML_CLEANER_H
#define __QUTE_NOTE__CORE__TOOLS__HTML_CLEANER_H

#include <tools/qt4helper.h>
#include <QString>

namespace qute_note {

class HTMLCleaner
{
public:
    HTMLCleaner();
    virtual ~HTMLCleaner();

    bool htmlToXml(const QString & html, QString & output, QString & errorDescription);
    bool htmlToXhtml(const QString & html, QString & output, QString & errorDescription);
    bool cleanupHtml(QString & html, QString & errorDescription);

private:
    HTMLCleaner(const HTMLCleaner & other) Q_DECL_DELETE;
    HTMLCleaner(HTMLCleaner && other) Q_DECL_DELETE;
    HTMLCleaner & operator=(const HTMLCleaner & other) Q_DECL_DELETE;
    HTMLCleaner & operator=(HTMLCleaner && other) Q_DECL_DELETE;

private:
    class Impl;
    Impl * m_impl;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__HTML_CLEANER_H

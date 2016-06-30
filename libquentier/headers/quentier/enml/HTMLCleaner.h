#ifndef LIB_QUENTIER_ENML_HTML_CLEANER_H
#define LIB_QUENTIER_ENML_HTML_CLEANER_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Qt4Helper.h>
#include <QString>

namespace quentier {

class QUENTIER_EXPORT HTMLCleaner
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

} // namespace quentier

#endif // LIB_QUENTIER_ENML_HTML_CLEANER_H

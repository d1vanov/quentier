#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__RESOURCE_INFO_JAVASCRIPT_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__RESOURCE_INFO_JAVASCRIPT_HANDLER_H

#include <QObject>
#include <QString>
#include <qute_note/utility/Qt4Helper.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceInfo)

/**
 * The ResourceInfoJavaScriptHandler is used for communicating the information on resources
 * from C++ to JavaScript on requests coming from JavaScript to C++
 */
class ResourceInfoJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit ResourceInfoJavaScriptHandler(const ResourceInfo & resourceInfo,
                                           QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void notifyResourceInfo(const QString & resourceHash,
                            const QString & resourceLocalFilePath,
                            const QString & resourceDisplayName,
                            const QString & resourceDisplaySize);

public Q_SLOTS:
    void findResourceInfo(const QString & resourceHash);

private:
    const ResourceInfo & m_resourceInfo;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__RESOURCE_INFO_JAVASCRIPT_HANDLER_H

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_RESOURCE_INFO_JAVASCRIPT_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_RESOURCE_INFO_JAVASCRIPT_HANDLER_H

#include <QObject>
#include <QString>
#include <quentier/utility/Qt4Helper.h>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_RESOURCE_INFO_JAVASCRIPT_HANDLER_H

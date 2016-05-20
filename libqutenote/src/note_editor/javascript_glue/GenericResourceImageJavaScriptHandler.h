#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_IMAGE_JAVA_SCRIPT_HANDLER_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_IMAGE_JAVA_SCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QHash>

namespace qute_note {

class GenericResourceImageJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit GenericResourceImageJavaScriptHandler(const QHash<QByteArray, QString> & cache, QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void genericResourceImageFound(QByteArray resourceHash, QString genericResourceImageFilePath);

public Q_SLOTS:
    void findGenericResourceImage(QByteArray resourceHash);

private:
    const QHash<QByteArray, QString> & m_cache;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_IMAGE_JAVA_SCRIPT_HANDLER_H

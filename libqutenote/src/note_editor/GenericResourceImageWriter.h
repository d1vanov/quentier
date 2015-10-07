#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__GENERIC_RESOURCE_IMAGE_WRITER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__GENERIC_RESOURCE_IMAGE_WRITER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QUuid>

namespace qute_note {

/**
 * @brief The GenericResourceImageWriter class is simply the I/O thread worker which
 * would write two files in a folder accessible for note editor's page: the composed image
 * of generic resource and the hash of that resource.
 */
class GenericResourceImageWriter: public QObject
{
    Q_OBJECT
public:
    explicit GenericResourceImageWriter(QObject * parent = Q_NULLPTR);

    void setStorageFolderPath(const QString & storageFolderPath);

Q_SIGNALS:
    void genericResourceImageWriteReply(const bool success, const QString errorDescription, const QUuid requestId);

public Q_SLOTS:
    void onGenericResourceImageWriteRequest(const QString resourceLocalGuid, const QByteArray resourceImageData,
                                            const QString resourceFileSuffix, const QString resourceActualHash,
                                            const QUuid requestId);

private:
    QString     m_storageFolderPath;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__GENERIC_RESOURCE_IMAGE_WRITER_H

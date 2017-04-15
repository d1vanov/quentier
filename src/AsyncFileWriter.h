#ifndef QUENTIER_ASYNC_FILE_WRITER_H
#define QUENTIER_ASYNC_FILE_WRITER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QObject>
#include <QRunnable>
#include <QString>

namespace quentier {

class AsyncFileWriter: public QObject,
                       public QRunnable
{
    Q_OBJECT
public:
    explicit AsyncFileWriter(const QString & filePath, const QByteArray & dataToWrite,
                             QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void fileSuccessfullyWritten(QString filePath);
    void fileWriteFailed(ErrorString error);
    void fileWriteIncomplete(const qint64 bytesWritten,
                             const qint64 bytesTotal);

private:
    virtual void run() Q_DECL_OVERRIDE;

private:
    QString     m_filePath;
    QByteArray  m_dataToWrite;
};

} // namespace quentier

#endif // QUENTIER_ASYNC_FILE_WRITER_H

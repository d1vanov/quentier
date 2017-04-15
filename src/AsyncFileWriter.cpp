#include "AsyncFileWriter.h"
#include <quentier/logging/QuentierLogger.h>
#include <QFile>

namespace quentier {

AsyncFileWriter::AsyncFileWriter(const QString & filePath,
                                 const QByteArray & dataToWrite,
                                 QObject * parent) :
    QObject(parent),
    QRunnable(),
    m_filePath(filePath),
    m_dataToWrite(dataToWrite)
{}

void AsyncFileWriter::run()
{
    QNDEBUG(QStringLiteral("AsyncFileWriter::run: file path = ") << m_filePath);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't open file for writing"));
        error.details() = file.errorString();
        QNWARNING(error);
        emit
    }

    qint64 bytesWritten = file.write(m_dataToWrite);
    if (bytesWritten != m_dataToWrite.size()) {
        emit fileWriteIncomplete(bytesWritten, m_dataToWrite.size());
        return;
    }

    emit fileSuccessfullyWritten(m_filePath);
}

} // namespace quentier

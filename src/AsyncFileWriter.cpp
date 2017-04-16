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

    qint64 dataSize = static_cast<qint64>(m_dataToWrite.size());
    qint64 bytesWritten = file.write(m_dataToWrite);
    if (bytesWritten != dataSize) {
        QNDEBUG(QStringLiteral("Couldn't write the entire file: expected ")
                << dataSize << QStringLiteral(", got only ") << bytesWritten);
        emit fileWriteIncomplete(bytesWritten, dataSize);
    }
    else {
        QNDEBUG(QStringLiteral("Successfully written the file"));
        emit fileSuccessfullyWritten(m_filePath);
    }
}

} // namespace quentier

#ifndef LIB_QUENTIER_LOGGING_CUSTOM_QUENTIER_LOGGER_H
#define LIB_QUENTIER_LOGGING_CUSTOM_QUENTIER_LOGGER_H

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Qt4Helper.h>
#include <QObject>
#include <QString>
#include <QFile>
#include <QVector>
#include <QPointer>
#include <cstddef>

namespace quentier {

/**
 * @brief The IQuentierLogWriter class is the interface for any class willing to implement a log writer
 *
 * Typically a particular log writer implements a particular logging destination, like file or stderr or anything else
 */
class IQuentierLogWriter: public QObject
{
    Q_OBJECT
public:
    IQuentierLogWriter(QObject * parent = Q_NULLPTR) :
        QObject(parent)
    {}

public Q_SLOTS:
    virtual void write(QString message) = 0;
};

/**
 * Type-safe max size of a log file in bytes
 */
class MaxSizeBytes
{
public:
    MaxSizeBytes(const std::size_t size) :
        m_size(size)
    {}

    std::size_t size() const { return m_size; }

private:
    std::size_t     m_size;
};

/**
 * Type-safe max number of old log files to keep around
 */
class MaxOldLogFilesCount
{
public:
    MaxOldLogFilesCount(const std::size_t count) :
        m_count(count)
    {}

    std::size_t count() const { return m_count; }

private:
    std::size_t     m_count;
};

/**
 * @brief The QuentierFileLogWriter class implements the log writer to a log file destination
 *
 * It features the automatic rotation of the log file by its max size and ensures not more than just a handful
 * of previous log files are stored around
 */
class QuentierFileLogWriter: public IQuentierLogWriter
{
public:
    QuentierFileLogWriter(const QString & filename, const MaxSizeBytes & maxSizeBytes,
                          const MaxOldLogFilesCount & maxOldLogFilesCount);
    ~QuentierFileLogWriter();

public Q_SLOTS:
    virtual void write(QString message) Q_DECL_OVERRIDE;

private:
    QFile               m_logFile;
    MaxSizeBytes        m_maxSizeBytes;
    MaxOldLogFilesCount m_maxOldLogFilesCount;
};

class QuentierLogger: public QObject
{
    Q_OBJECT
public:
    static QuentierLogger & instance();

    void addLogWriter(IQuentierLogWriter * pWriter);

private:
    QuentierLogger();
    Q_DISABLE_COPY(QuentierLogger)

private:
    QVector<QPointer<IQuentierLogWriter> >  m_logWriterPtrs;
    LogLevel::type      m_minLogLevel;
};

} // namespace quentier

#endif // LIB_QUENTIER_LOGGING_CUSTOM_QUENTIER_LOGGER_H

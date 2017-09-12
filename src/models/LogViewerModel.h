/*
 * Copyright 2017 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_MODELS_LOG_VIEWER_MODEL_H
#define QUENTIER_MODELS_LOG_VIEWER_MODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Printable.h>
#include <quentier/types/ErrorString.h>
#include <QAbstractTableModel>
#include <QFile>
#include <QVector>

namespace quentier {

class LogViewerModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    LogViewerModel(QObject * parent = Q_NULLPTR);

    struct Columns
    {
        enum type {
            SourceFileName = 0,
            SourceFileLineNumber,
            LogLevel,
            LogEntry
        };
    };

    QString logFileName() const;
    void setLogFileName(const QString & logFileName);

    qint64 offsetPos() const { return m_offsetPos; }
    void setOffsetPos(const qint64 offsetPos);

    void copyAllToClipboard();

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

private:
    // QAbstractTableModel interface
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

private:
    void parseFullDataFromLogFile();
    void parseDataFromLogFileFromCurrentPos();

private:
    Q_DISABLE_COPY(LogViewerModel)

private:
    QFile       m_currentLogFile;
    qint64      m_currentPos;
    qint64      m_offsetPos;

    struct Data: public Printable
    {
        Data() :
            m_sourceFileName(),
            m_sourceFileLineNumber(-1),
            m_logLevel(LogLevel::InfoLevel),
            m_logEntry()
        {}

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

        QString         m_sourceFileName;
        int             m_sourceFileLineNumber;
        LogLevel::type  m_logLevel;
        QString         m_logEntry;
    };

    QVector<Data>   m_data;
};

} // namespace quentier

#endif // QUENTIER_MODELS_LOG_VIEWER_MODEL_H

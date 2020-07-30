/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_CRASH_HANDLER_SYMBOLS_UNPACKER_H
#define QUENTIER_CRASH_HANDLER_SYMBOLS_UNPACKER_H

#include <quentier/utility/Macros.h>

#include <QByteArray>
#include <QObject>
#include <QRunnable>
#include <QString>

class SymbolsUnpacker: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit SymbolsUnpacker(
        const QString & compressedSymbolsFilePath,
        const QString & unpackedSymbolsRootPath,
        QObject * parent = nullptr);

    virtual ~SymbolsUnpacker() override;

    virtual void run() override;

Q_SIGNALS:
    void finished(bool status, QString errorDescription);

private:
    QString     m_compressedSymbolsFilePath;
    QString     m_unpackedSymbolsRootPath;
};

#endif // QUENTIER_CRASH_HANDLER_SYMBOLS_UNPACKER_H

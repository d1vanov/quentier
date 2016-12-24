/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/utility/SysInfo.h>
#include "../SysInfo_p.h"
#include "StackTrace.h"
#include <unistd.h>
#include <QString>
#include <QFile>
#include <QByteArray>
#include <QMutexLocker>
#include <QDir>
#include <QApplication>

namespace quentier {

qint64 SysInfo::pageSize()
{
    return static_cast<qint64>(sysconf(_SC_PAGESIZE));
}

QString SysInfo::stackTrace()
{
    Q_D(SysInfo);
    QMutexLocker mutexLocker(&d->m_mutex);

    fpos_t pos;

    QString tmpFile = QDir::tempPath();
    QString appName = QApplication::applicationName();

    tmpFile += QStringLiteral("/Quentier_") + appName + QStringLiteral("_StackTrace.txt");

    // flush existing stderr and reopen it as file
    fflush(stderr);
    fgetpos(stderr, &pos);
    int fd = dup(fileno(stderr));
    FILE * fileHandle = freopen(tmpFile.toLocal8Bit().data(), "w", stderr);
    if (!fileHandle) {
        perror("Can't reopen stderr");
        return QString();
    }

    stacktrace::displayCurrentStackTrace();

    // revert stderr
    fflush(stderr);
    dup2(fd, fileno(stderr));
    close(fd);
    clearerr(stderr);
    fsetpos(stderr, &pos);
    fclose(fileHandle);

    QFile file(tmpFile);
    bool res = file.open(QIODevice::ReadOnly);
    if (!res) {
        return QStringLiteral("Cannot open temporary file with stacktrace");
    }

    QByteArray rawOutput = file.readAll();
    QString output(rawOutput);
    return output;
}

} // namespace quentier

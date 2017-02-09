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

#include "NoteThumbnailDownloader.h"
#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <qt4qevercloud/thumbnail.h>
#else
#include <qt5qevercloud/thumbnail.h>
#endif

#include <QFile>
#include <QFileInfo>
#include <QDir>

namespace quentier {

NoteThumbnailDownloader::NoteThumbnailDownloader(const QString & host, const QString & noteGuid,
                                                 const QString & authToken, const QString & shardId,
                                                 const bool noteFromPublicLinkedNotebook,
                                                 QObject * parent) :
    QObject(parent),
    m_host(host),
    m_noteGuid(noteGuid),
    m_authToken(authToken),
    m_shardId(shardId),
    m_noteFromPublicLinkedNotebook(noteFromPublicLinkedNotebook)
{}

void NoteThumbnailDownloader::run()
{
    QNDEBUG(QStringLiteral("NoteThumbnailDownloader::run: host = ") << m_host << QStringLiteral(", note guid = ")
            << m_noteGuid << QStringLiteral(", is public = ") << (m_noteFromPublicLinkedNotebook ? QStringLiteral("true") : QStringLiteral("false")));

#define SET_ERROR(error) \
    ErrorString errorDescription(error); \
    emit finished(false, m_noteGuid, QString(), errorDescription); \
    return

    if (Q_UNLIKELY(m_host.isEmpty())) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "host is empty"));
    }

    if (Q_UNLIKELY(m_noteGuid.isEmpty())) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "note guid is empty"));
    }

    if (Q_UNLIKELY(m_shardId.isEmpty())) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "shard id is empty"));
    }

    if (Q_UNLIKELY(!m_noteFromPublicLinkedNotebook && m_authToken.isEmpty())) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "authentication data is incomplete"));
    }

    qevercloud::Thumbnail thumbnail(m_host, m_shardId, m_authToken);
    QByteArray thumbnailImageData = thumbnail.download(m_noteGuid, m_noteFromPublicLinkedNotebook,
                                                       /* is resource guid = */ false);
    if (Q_UNLIKELY(thumbnailImageData.isEmpty())) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "received empty note thumbnail data"));
    }

    QString folderPath = applicationPersistentStoragePath() + QStringLiteral("/NoteEditorPage/thumbnails");
    QFileInfo folderPathInfo(folderPath);
    if (Q_UNLIKELY(!folderPathInfo.exists()))
    {
        QDir dir(folderPath);
        bool res = dir.mkpath(folderPath);
        if (Q_UNLIKELY(!res)) {
            SET_ERROR(QT_TRANSLATE_NOOP("", "can't create folder to store note thumbnails in"));
        }
    }
    else if (Q_UNLIKELY(!folderPathInfo.isDir())) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "can't create the folder to store the note thumbnails in: "
                                    "a file with similar name and path exists"));
    }
    else if (Q_UNLIKELY(!folderPathInfo.isWritable())) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "folder to store note thumbnails in is not writable"));
    }

    QString filePath = folderPath + QStringLiteral("/") + m_noteGuid;
    QFile file(filePath);
    if (Q_UNLIKELY(!file.open(QIODevice::WriteOnly))) {
        SET_ERROR(QT_TRANSLATE_NOOP("", "can't open file to write the note thumbnail into"));
    }

    file.write(thumbnailImageData);
    file.close();

    emit finished(true, m_noteGuid, filePath, ErrorString());
}

} // namespace quentier

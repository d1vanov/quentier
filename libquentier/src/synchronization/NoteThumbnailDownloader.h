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

#ifndef LIB_QUENTIER_SYNCHRONIZATION_NOTE_THUMBNAIL_DOWNLOADER_H
#define LIB_QUENTIER_SYNCHRONIZATION_NOTE_THUMBNAIL_DOWNLOADER_H

#include <QObject>
#include <QRunnable>
#include <QString>
#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>

namespace quentier {

class NoteThumbnailDownloader: public QObject,
                               public QRunnable
{
    Q_OBJECT
public:
    explicit NoteThumbnailDownloader(const QString & host, const QString & noteGuid,
                                     const QString & authToken, const QString & shardId,
                                     const bool noteFromPublicLinkedNotebook,
                                     QObject * parent = Q_NULLPTR);

    virtual void run() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void finished(bool success, QString noteGuid, QString downloadedThumbnailImageFilePath, QNLocalizedString errorDescription);

private:
    QString     m_host;
    QString     m_noteGuid;
    QString     m_authToken;
    QString     m_shardId;
    bool        m_noteFromPublicLinkedNotebook;
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_NOTE_THUMBNAIL_DOWNLOADER_H

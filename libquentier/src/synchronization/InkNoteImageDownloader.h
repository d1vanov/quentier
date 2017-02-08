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

#ifndef LIB_QUENTIER_SYNCHRONIZATION_INK_NOTE_IMAGE_DOWNLOADER_H
#define LIB_QUENTIER_SYNCHRONIZATION_INK_NOTE_IMAGE_DOWNLOADER_H

#include <QObject>
#include <QRunnable>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/Macros.h>

namespace quentier {

class InkNoteImageDownloader: public QObject,
                              public QRunnable
{
    Q_OBJECT
public:
    explicit InkNoteImageDownloader(const QString & host, const QString & resourceGuid,
                                    const QString & authToken, const QString & shardId,
                                    const int height, const int width,
                                    const bool noteFromPublicLinkedNotebook,
                                    QObject * parent = Q_NULLPTR);

    virtual void run() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void finished(bool status, QString inkNoteImageFilePath, ErrorString errorDescription);

private:
    QString     m_host;
    QString     m_resourceGuid;
    QString     m_authToken;
    QString     m_shardId;
    int         m_height;
    int         m_width;
    bool        m_noteFromPublicLinkedNotebook;
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_INK_NOTE_IMAGE_DOWNLOADER_H

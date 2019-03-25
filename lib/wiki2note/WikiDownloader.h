/*
 * Copyright 2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIKI2NOTE_WIKI_DOWNLOADER_H
#define QUENTIER_LIB_WIKI2NOTE_WIKI_DOWNLOADER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Note.h>

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QUuid>
#include <QUrl>

namespace quentier {

class WikiDownloader: public QObject
{
    Q_OBJECT
public:
    explicit WikiDownloader(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void finished(bool status, Note note, QUuid requestId);

public Q_SLOTS:
    void download(QUuid requestId, QString query);

private:
    Q_DISABLE_COPY(WikiDownloader);

private:
};

} // namespace quentier

#endif // QUENTIER_LIB_WIKI2NOTE_WIKI_DOWNLOADER_H

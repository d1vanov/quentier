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

#ifndef QUENTIER_WIKI2ACCOUNT_TAG_CONTROLLER_H
#define QUENTIER_WIKI2ACCOUNT_TAG_CONTROLLER_H

#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/Tag.h>

#include <QUuid>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)

class TagController: public QObject
{
    Q_OBJECT
public:
    explicit TagController(const quint32 minTagsPerNote,
                           const quint32 maxTagsPerNote,
                           LocalStorageManagerAsync & localStorageManagerAsync,
                           QObject * parent = Q_NULLPTR);
    virtual ~TagController();

    const QList<Tag> & tags() const { return m_tags; }

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

    // private signals
    void addTag(Tag tag, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void clear();

    void findNextTag();
    void createNextNewTag();

    QString nextNewTagName();

private:
    quint32     m_minTagsPerNote;
    quint32     m_maxTagsPerNote;

    QList<Tag>  m_tags;

    QUuid       m_findTagRequestId;
    QUuid       m_addTagRequestId;

    quint32     m_nextNewTagNameSuffix;
};

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_TAG_CONTROLLER_H

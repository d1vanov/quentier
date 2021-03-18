/*
 * Copyright 2019-2021 Dmitry Ivanov
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

#include <qevercloud/generated/types/Tag.h>

#include <QUuid>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)

class TagController final : public QObject
{
    Q_OBJECT
public:
    explicit TagController(
        quint32 minTagsPerNote, quint32 maxTagsPerNote,
        LocalStorageManagerAsync & localStorageManagerAsync,
        QObject * parent = nullptr);

    ~TagController() override;

    [[nodiscard]] const QList<qevercloud::Tag> & tags() const noexcept
    {
        return m_tags;
    }

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

    // private signals
    void addTag(qevercloud::Tag tag, QUuid requestId);
    void findTag(qevercloud::Tag tag, QUuid requestId);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onAddTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onAddTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagComplete(qevercloud::Tag tag, QUuid requestId);
    void onFindTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void clear();

    void findNextTag();
    void createNextNewTag();

    [[nodiscard]] QString nextNewTagName() const;

private:
    quint32 m_minTagsPerNote;
    quint32 m_maxTagsPerNote;

    QList<qevercloud::Tag> m_tags;

    QUuid m_findTagRequestId;
    QUuid m_addTagRequestId;

    quint32 m_nextNewTagNameSuffix = 1;
};

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_TAG_CONTROLLER_H

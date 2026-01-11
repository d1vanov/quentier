/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/cancelers/Fwd.h>

#include <qevercloud/types/Tag.h>

#include <QList>

namespace quentier {

class TagController : public QObject
{
    Q_OBJECT
public:
    explicit TagController(
        quint32 minTagsPerNote, quint32 maxTagsPerNote,
        local_storage::ILocalStoragePtr localStorage,
        QObject * parent = nullptr);

    ~TagController() override;

    [[nodiscard]] const QList<qevercloud::Tag> & tags() const noexcept
    {
        return m_tags;
    }

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

public Q_SLOTS:
    void start();

private:
    void clear();

    void findNextTag();
    void createNextNewTag();
    [[nodiscard]] QString nextNewTagName();

    [[nodiscard]] utility::cancelers::ICancelerPtr setupCanceler();

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    const quint32 m_minTagsPerNote;
    const quint32 m_maxTagsPerNote;

    QList<qevercloud::Tag> m_tags;
    quint32 m_nextNewTagNameSuffix = 1;

    utility::cancelers::ManualCancelerPtr m_canceler;
};

} // namespace quentier

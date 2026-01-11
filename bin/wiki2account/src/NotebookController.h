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

#include <qevercloud/types/Notebook.h>

#include <QHash>
#include <QList>
#include <QObject>

namespace quentier {

/**
 * @brief The NotebookController class processes the notebook options for
 * wiki2account utility
 *
 * If target notebook name was specified, it ensures that such notebook exists
 * within the account. If number of new notebooks was specified, this class
 * creates these notebooks and returns them - with names and local uids
 */
class NotebookController : public QObject
{
    Q_OBJECT
public:
    explicit NotebookController(
        QString targetNotebookName, quint32 numNotebooks,
        local_storage::ILocalStoragePtr localStorage,
        QObject * parent = nullptr);

    ~NotebookController() override;

    [[nodiscard]] const qevercloud::Notebook & targetNotebook() const noexcept
    {
        return m_targetNotebook;
    }

    [[nodiscard]] const QList<qevercloud::Notebook> & newNotebooks()
        const noexcept
    {
        return m_newNotebooks;
    }

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

public Q_SLOTS:
    void start();

private:
    void clear();
    void createNextNewNotebook();

    [[nodiscard]] utility::cancelers::ICancelerPtr setupCanceler();

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    const QString m_targetNotebookName;
    const quint32 m_numNewNotebooks;

    QHash<QString, QString> m_notebookLocalIdsByNames;

    qevercloud::Notebook m_targetNotebook;
    QList<qevercloud::Notebook> m_newNotebooks;
    qint32 m_lastNewNotebookIndex = 1;

    utility::cancelers::ManualCancelerPtr m_canceler;
};

} // namespace quentier

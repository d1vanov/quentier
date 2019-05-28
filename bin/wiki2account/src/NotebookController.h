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

#include <quentier/utility/Macros.h>
#include <quentier/types/Notebook.h>

#include <QList>
#include <QObject>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)

/**
 * @brief The NotebookController class processes the notebook options for
 * wiki2account utility
 *
 * If target notebook name was specified, it ensures that such notebook exists
 * within the account. If number of new notebooks was specified, this class
 * creates these notebooks and returns them - with names and local uids
 */
class NotebookController: public QObject
{
    Q_OBJECT
public:
    explicit NotebookController(const QString & targetNotebookName,
                                const quint32 numNotebooks,
                                LocalStorageManagerAsync & localStorageManagerAsync,
                                QObject * parent = Q_NULLPTR);
    virtual ~NotebookController();

    Notebook targetNotebook() const;
    QList<Notebook> newNotebooks() const;

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

public Q_SLOTS:
    void start();
};

} // namespace quentier

/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_TAG_LINKED_NOTEBOOK_ROOT_ITEM_H
#define QUENTIER_LIB_MODEL_TAG_LINKED_NOTEBOOK_ROOT_ITEM_H

#include <quentier/utility/Printable.h>

namespace quentier {

class TagLinkedNotebookRootItem: public Printable
{
public:
    TagLinkedNotebookRootItem(
            const QString & username,
            const QString & linkedNotebookGuid) :
        m_username(username),
        m_linkedNotebookGuid(linkedNotebookGuid)
    {}

    const QString & username() const { return m_username; }
    void setUsername(const QString & username) { m_username = username; }

    const QString & linkedNotebookGuid() const { return m_linkedNotebookGuid; }
    void setLinkedNotebookGuid(const QString & linkedNotebookGuid)
    { m_linkedNotebookGuid = linkedNotebookGuid; }

    virtual QTextStream & print(QTextStream & strm) const override;

private:
    QString         m_username;
    QString         m_linkedNotebookGuid;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_LINKED_NOTEBOOK_ROOT_ITEM_H

/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_TAG_ITEM_H
#define QUENTIER_MODELS_TAG_ITEM_H

#include <quentier/utility/Printable.h>

namespace quentier {

class TagItem: public Printable
{
public:
    explicit TagItem(const QString & localUid = QString(),
                     const QString & guid = QString(),
                     const QString & linkedNotebookGuid = QString(),
                     const QString & name = QString(),
                     const QString & parentLocalUid = QString(),
                     const QString & parentGuid = QString(),
                     const bool isSynchronizable = false,
                     const bool isDirty = false,
                     const bool isFavorited = false,
                     const int numNotesPerTag = -1);
    virtual ~TagItem();

    const QString & localUid() const { return m_localUid; }
    void setLocalUid(const QString & localUid) { m_localUid = localUid; }

    const QString & guid() const { return m_guid; }
    void setGuid(const QString & guid) { m_guid = guid; }

    const QString & linkedNotebookGuid() const { return m_linkedNotebookGuid; }
    void setLinkedNotebookGuid(const QString & linkedNotebookGuid) { m_linkedNotebookGuid = linkedNotebookGuid; }

    QString nameUpper() const { return m_name.toUpper(); }

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    const QString & parentGuid() const { return m_parentGuid; }
    void setParentGuid(const QString & parentGuid) { m_parentGuid = parentGuid; }

    const QString & parentLocalUid() const { return m_parentLocalUid; }
    void setParentLocalUid(const QString & parentLocalUid) { m_parentLocalUid = parentLocalUid; }

    bool isSynchronizable() const { return m_isSynchronizable; }
    void setSynchronizable(const bool synchronizable) { m_isSynchronizable = synchronizable; }

    bool isDirty() const { return m_isDirty; }
    void setDirty(const bool dirty) { m_isDirty = dirty; }

    bool isFavorited() const { return m_isFavorited; }
    void setFavorited(const bool favorited) { m_isFavorited = favorited; }

    int numNotesPerTag() const { return m_numNotesPerTag; }
    void setNumNotesPerTag(const int numNotesPerTag) { m_numNotesPerTag = numNotesPerTag; }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString     m_localUid;
    QString     m_guid;
    QString     m_linkedNotebookGuid;
    QString     m_name;
    QString     m_parentLocalUid;
    QString     m_parentGuid;
    bool        m_isSynchronizable;
    bool        m_isDirty;
    bool        m_isFavorited;
    int         m_numNotesPerTag;
};

} // namespace quentier

#endif // QUENTIER_MODELS_TAG_ITEM_H

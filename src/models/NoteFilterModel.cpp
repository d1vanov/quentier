/*
 * Copyright 2016 Dmitry Ivanov
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

#include "NoteFilterModel.h"
#include "NoteModel.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

NoteFilterModel::NoteFilterModel(QObject * parent) :
    QSortFilterProxyModel(parent),
    m_notebookLocalUids(),
    m_tagNames(),
    m_noteLocalUids(),
    m_pendingFilterUpdate(false),
    m_modifiedWhilePendingFilterUpdate(false)
{}

void NoteFilterModel::setNotebookLocalUids(const QStringList & notebookLocalUids)
{
    QNDEBUG(QStringLiteral("NoteFilterModel::setNotebookLocalUids: ") << notebookLocalUids.join(QStringLiteral(", ")));

    if (m_noteLocalUids.isEmpty() && (m_notebookLocalUids.size() == notebookLocalUids.size()))
    {
        bool foundDifference = false;
        for(auto it = m_notebookLocalUids.constBegin(), end = m_notebookLocalUids.constEnd(); it != end; ++it)
        {
            if (!notebookLocalUids.contains(*it)) {
                foundDifference = true;
                break;
            }
        }

        if (!foundDifference) {
            QNTRACE(QStringLiteral("The same set of notebook local uids is set currently, nothing has changed"));
            return;
        }
    }

    m_notebookLocalUids = notebookLocalUids;
    m_noteLocalUids.clear();

    if (!m_pendingFilterUpdate) {
        QSortFilterProxyModel::invalidateFilter();
    }
    else {
        m_modifiedWhilePendingFilterUpdate = true;
    }
}

void NoteFilterModel::setTagNames(const QStringList & tagNames)
{
    QNDEBUG(QStringLiteral("NoteFilterModel::setTagNames: ") << tagNames.join(QStringLiteral(", ")));

    if (m_noteLocalUids.isEmpty() && (m_tagNames.size() == tagNames.size()))
    {
        bool foundDifference = false;
        for(auto it = m_tagNames.constBegin(), end = m_tagNames.constEnd(); it != end; ++it)
        {
            if (!tagNames.contains(*it)) {
                foundDifference = true;
                break;
            }
        }

        if (!foundDifference) {
            QNTRACE(QStringLiteral("The same set of tag names is set currently, nothing has changed"));
            return;
        }
    }

    m_tagNames = tagNames;
    m_noteLocalUids.clear();

    if (!m_pendingFilterUpdate) {
        QSortFilterProxyModel::invalidateFilter();
    }
    else {
        m_modifiedWhilePendingFilterUpdate = true;
    }
}

void NoteFilterModel::setNoteLocalUids(const QStringList & noteLocalUids)
{
    QNDEBUG(QStringLiteral("NoteFilterModel::setNoteLocalUids: ") << noteLocalUids.join(QStringLiteral(", ")));

    if (m_noteLocalUids.size() == noteLocalUids.size())
    {
        bool foundDifference = false;
        for(auto it = m_noteLocalUids.constBegin(), end = m_noteLocalUids.constEnd(); it != end; ++it)
        {
            if (!noteLocalUids.contains(*it)) {
                foundDifference = true;
                break;
            }
        }

        if (!foundDifference) {
            QNTRACE(QStringLiteral("The same set of note local uids is set currently, nothing has changed"));
            return;
        }
    }

    m_noteLocalUids = noteLocalUids;

    if (!m_pendingFilterUpdate) {
        QSortFilterProxyModel::invalidateFilter();
    }
    else {
        m_modifiedWhilePendingFilterUpdate = true;
    }
}

void NoteFilterModel::beginUpdateFilter()
{
    QNDEBUG(QStringLiteral("NoteFilterModel::beginUpdateFilter"));
    m_pendingFilterUpdate = true;
}

void NoteFilterModel::endUpdateFilter()
{
    QNDEBUG(QStringLiteral("NoteFilterModel::endUpdateFilter"));

    m_pendingFilterUpdate = false;

    if (m_modifiedWhilePendingFilterUpdate) {
        m_modifiedWhilePendingFilterUpdate = false;
        QNTRACE(QStringLiteral("Invalidating the note filter"));
        QSortFilterProxyModel::invalidateFilter();
    }
}

QTextStream & NoteFilterModel::print(QTextStream & strm) const
{
    strm << QStringLiteral("NoteFilterModel: {\n");
    strm << QStringLiteral("    notebook local uids: ")
         << (m_notebookLocalUids.isEmpty() ? QStringLiteral("<empty>") : m_notebookLocalUids.join(QStringLiteral(", ")))
         << QStringLiteral(";\n");
    strm << QStringLiteral("    tag names: ")
         << (m_tagNames.isEmpty() ? QStringLiteral("<empty>") : m_tagNames.join(QStringLiteral(", ")))
         << QStringLiteral(";\n");
    strm << QStringLiteral("    note local uids: ")
         << (m_noteLocalUids.isEmpty() ? QStringLiteral("<empty>") : m_noteLocalUids.join(QStringLiteral(", ")))
         << QStringLiteral(";\n");
    strm << QStringLiteral("    pending filter update: ") << (m_pendingFilterUpdate ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(";\n");
    strm << QStringLiteral("    modified while pending filter update: ")
         << (m_modifiedWhilePendingFilterUpdate ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(";\n");
    strm << QStringLiteral("};\n");
    return strm;
}

bool NoteFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const
{
    Q_UNUSED(sourceParent);

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(QSortFilterProxyModel::sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Internal error: failed to get the note model from its proxy filter model"));
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    const NoteModelItem * pItem = pNoteModel->itemAtRow(sourceRow);
    if (Q_UNLIKELY(!pItem)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Failed to get get note model item at the specified row"));
        error.details() = QString::number(sourceRow);
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    // NOTE: filtering by note local uids overrides filtering by notebooks and tags
    if (!m_noteLocalUids.isEmpty()) {
        return m_noteLocalUids.contains(pItem->localUid());
    }

    // NOTE: filtering by notebooks and tags is cumulative: the row is only accepted if it's accepted by both
    // notebook and tag filters (if both are set)

    if (!m_notebookLocalUids.isEmpty())
    {
        bool filteredIn = m_notebookLocalUids.contains(pItem->notebookLocalUid());
        if (!filteredIn) {
            return false;
        }
    }

    if (!m_tagNames.isEmpty())
    {
        const QStringList & itemTagNames = pItem->tagNameList();
        for(auto it = itemTagNames.constBegin(), end = itemTagNames.constEnd(); it != end; ++it)
        {
            if (m_tagNames.contains(*it)) {
                return true;
            }
        }

        return false;
    }

    return true;
}

} // namespace quentier

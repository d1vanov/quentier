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
    m_noteLocalUids()
{}

void NoteFilterModel::setNotebookLocalUids(const QStringList & notebookLocalUids)
{
    m_notebookLocalUids = notebookLocalUids;
    m_tagNames.clear();
    m_noteLocalUids.clear();
    QSortFilterProxyModel::invalidateFilter();
}

void NoteFilterModel::setTagNames(const QStringList & tagNames)
{
    m_tagNames = tagNames;
    m_notebookLocalUids.clear();
    m_notebookLocalUids.clear();
    QSortFilterProxyModel::invalidateFilter();
}

void NoteFilterModel::setNoteLocalUids(const QStringList & noteLocalUids)
{
    m_noteLocalUids = noteLocalUids;
    m_notebookLocalUids.clear();
    m_tagNames.clear();
    QSortFilterProxyModel::invalidateFilter();
}

QTextStream & NoteFilterModel::print(QTextStream & strm) const
{
    strm << "NoteFilterModel: {\n";
    strm << "    notebook local uids: "
         << (m_notebookLocalUids.isEmpty() ? QStringLiteral("<empty>") : m_notebookLocalUids.join(", "))
         << ";\n";
    strm << "    tag names: "
         << (m_tagNames.isEmpty() ? QStringLiteral("<empty>") : m_tagNames.join(", "))
         << ";\n";
    strm << "    note local uids: "
         << (m_noteLocalUids.isEmpty() ? QStringLiteral("<empty>") : m_noteLocalUids.join(", "))
         << ";\n";
    strm << "};\n";
    return strm;
}

bool NoteFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const
{
    Q_UNUSED(sourceParent);

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(QSortFilterProxyModel::sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNLocalizedString error = QT_TR_NOOP("can't access note model from the proxy filter model");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    const NoteModelItem * pItem = pNoteModel->itemAtRow(sourceRow);
    if (Q_UNLIKELY(!pItem)) {
        QNLocalizedString error = QT_TR_NOOP("can't get note model item at row");
        error += " ";
        error += QString::number(sourceRow);
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    if (!m_notebookLocalUids.isEmpty()) {
        return m_notebookLocalUids.contains(pItem->notebookLocalUid());
    }
    else if (!m_tagNames.isEmpty()) {
        const QStringList & itemTagNames = pItem->tagNameList();
        for(auto it = itemTagNames.begin(), end = itemTagNames.end(); it != end; ++it) {
            if (m_tagNames.contains(*it)) {
                return true;
            }
        }
        return false;
    }
    else if (!m_noteLocalUids.isEmpty()) {
        return m_noteLocalUids.contains(pItem->localUid());
    }

    return true;
}

} // namespace quentier

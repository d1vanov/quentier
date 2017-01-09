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

#ifndef QUENTIER_MODELS_NOTE_FILTER_NODEL_H
#define QUENTIER_MODELS_NOTE_FILTER_NODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/utility/Printable.h>
#include <QSortFilterProxyModel>

namespace quentier {

class NoteFilterModel: public QSortFilterProxyModel,
                       public Printable
{
    Q_OBJECT
public:
    explicit NoteFilterModel(QObject * parent = Q_NULLPTR);

    const QStringList & notebookLocalUids() const { return m_notebookLocalUids; }
    void setNotebookLocalUids(const QStringList & notebookLocalUids);

    const QStringList & tagNames() const { return m_tagNames; }
    void setTagNames(const QStringList & tagNames);

    const QStringList & noteLocalUids() const { return m_noteLocalUids; }
    void setNoteLocalUids(const QStringList & noteLocalUids);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(QNLocalizedString error) const;

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const Q_DECL_OVERRIDE;

private:
    QStringList m_notebookLocalUids;
    QStringList m_tagNames;
    QStringList m_noteLocalUids;
};

} // namespace quentier

#endif // QUENTIER_MODELS_NOTE_FILTER_NODEL_H

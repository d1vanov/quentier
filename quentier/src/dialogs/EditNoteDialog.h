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

#ifndef QUENTIER_DIALOGS_EDIT_NOTE_DIALOG_H
#define QUENTIER_DIALOGS_EDIT_NOTE_DIALOG_H

#include <quentier/types/Note.h>
#include <quentier/utility/Macros.h>
#include <QDialog>
#include <QPointer>

namespace Ui {
class EditNoteDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookModel)

class EditNoteDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EditNoteDialog(const Note & note, NotebookModel * pNotebookModel,
                            QWidget * parent = Q_NULLPTR);
    virtual ~EditNoteDialog();

    const Note & note() const { return m_note; }

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;

    // Slots to track the updates of notebook model
    void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            );
#else
                            , const QVector<int> & roles = QVector<int>());
#endif

    void rowsInserted(const QModelIndex & parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);

private:
    void createConnections();
    void fillNotebookNames();
    void fillDialogContent();

private:
    Ui::EditNoteDialog *    m_pUi;
    Note                    m_note;
    QPointer<NotebookModel> m_pNotebookModel;
    QStringListModel *      m_pNotebookNamesModel;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_EDIT_NOTE_DIALOG_H

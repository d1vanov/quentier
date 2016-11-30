#ifndef QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H
#define QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H

#include "models/NoteCache.h"
#include "models/NotebookCache.h"
#include "models/TagCache.h"
#include <quentier/types/Account.h>
#include <QTabWidget>

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(NoteEditorWidget)

class NoteEditorTabWidgetManager: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorTabWidgetManager(const Account & account, LocalStorageManagerThreadWorker & localStorageWorker,
                                        NoteCache & noteCache, NotebookCache & notebookCache,
                                        TagCache & tagCache, TagModel & tagModel,
                                        QTabWidget * tabWidget, QObject * parent = Q_NULLPTR);

    int maxNumNotes() const { return m_maxNumNotes; }
    void setMaxNumNotes(const int maxNumNotes);

    int numNotes() const;

    void addNote(const Note & note);

private:
    Account                             m_currentAccount;
    LocalStorageManagerThreadWorker &   m_localStorageWorker;
    NoteCache &                         m_noteCache;
    NotebookCache &                     m_notebookCache;
    TagCache &                          m_tagCache;
    TagModel &                          m_tagModel;

    QTabWidget *                        m_pTabWidget;
    NoteEditorWidget *                  m_pBlankNoteEditor;
    int                                 m_blankNoteTab;
    QUndoStack *                        m_pBlankNoteTabUndoStack;

    int                                 m_maxNumNotes;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H

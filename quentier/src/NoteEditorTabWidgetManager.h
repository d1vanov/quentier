#ifndef QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H
#define QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H

#include "models/NoteCache.h"
#include "models/NotebookCache.h"
#include "models/TagCache.h"
#include <quentier/types/Account.h>
#include <quentier/utility/LRUCache.hpp>
#include <QTabWidget>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/circular_buffer.hpp>
#endif

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(NoteEditorWidget)
QT_FORWARD_DECLARE_CLASS(TabWidget)

class NoteEditorTabWidgetManager: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorTabWidgetManager(const Account & account, LocalStorageManagerThreadWorker & localStorageWorker,
                                        NoteCache & noteCache, NotebookCache & notebookCache,
                                        TagCache & tagCache, TagModel & tagModel,
                                        TabWidget * tabWidget, QObject * parent = Q_NULLPTR);

    int maxNumNotes() const { return m_maxNumNotes; }
    void setMaxNumNotes(const int maxNumNotes);

    int numNotes() const;

    void addNote(const QString & noteLocalUid);

Q_SIGNALS:
    void notifyError(QNLocalizedString error);

private Q_SLOTS:
    void onNoteEditorWidgetResolved();
    void onNoteTitleOrPreviewTextChanged(QString titleOrPreview);

private:
    void insertNoteEditorWidget(NoteEditorWidget * pNoteEditorWidget);
    void checkAndCloseOlderNoteEditors();

private:
    Account                             m_currentAccount;
    LocalStorageManagerThreadWorker &   m_localStorageWorker;
    NoteCache &                         m_noteCache;
    NotebookCache &                     m_notebookCache;
    TagCache &                          m_tagCache;
    TagModel &                          m_tagModel;

    TabWidget *                         m_pTabWidget;
    NoteEditorWidget *                  m_pBlankNoteEditor;

    int                                 m_maxNumNotes;

    boost::circular_buffer<QString>     m_shownNoteLocalUids;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H

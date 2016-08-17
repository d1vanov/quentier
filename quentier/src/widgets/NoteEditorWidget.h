#ifndef QUENTIER_WIDGETS_NOTE_EDITOR_WIDGET_H
#define QUENTIER_WIDGETS_NOTE_EDITOR_WIDGET_H

#include "../models/NoteCache.h"
#include "../models/NotebookCache.h"
#include "../models/TagCache.h"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Tag.h>
#include <QWidget>
#include <QUuid>
#include <QScopedPointer>
#include <QStringList>

namespace Ui {
class NoteEditorWidget;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

/**
 * @brief The NoteEditorWidget class contains the actual note editor +
 * note title + toolbar with formatting options + debug html source view +
 * note tags widget
 */
class NoteEditorWidget: public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditorWidget(LocalStorageManagerThreadWorker & localStorageWorker,
                              NoteCache & noteCache, NotebookCache & notebookCache,
                              TagCache & TagCache, QWidget * parent = Q_NULLPTR);
    virtual ~NoteEditorWidget();

    void setNoteLocalUid(const QString & noteLocalUid);

Q_SIGNALS:
    void notifyError(QNLocalizedString error);

// private signals
    void updateNote(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

public Q_SLOTS:
    // Slots for toolbar button actions or external actions
    void onEditorTextBoldToggled();
    void onEditorTextItalicToggled();
    void onEditorTextUnderlineToggled();
    void onEditorTextStrikethroughToggled();
    void onEditorTextAlignLeftAction();
    void onEditorTextAlignCenterAction();
    void onEditorTextAlignRightAction();
    void onEditorTextAddHorizontalLineAction();
    void onEditorTextIncreaseFontSizeAction();
    void onEditorTextDecreaseFontSizeAction();
    void onEditorTextHighlightAction();
    void onEditorTextIncreaseIndentationAction();
    void onEditorTextDecreaseIndentationAction();
    void onEditorTextInsertUnorderedListAction();
    void onEditorTextInsertOrderedListAction();
    void onEditorTextEditHyperlinkAction();
    void onEditorTextCopyHyperlinkAction();
    void onEditorTextRemoveHyperlinkAction();

    void onEditorChooseTextColor(QColor color);
    void onEditorChooseBackgroundColor(QColor color);

    void onEditorSpellCheckStateChanged(int state);
    void onEditorInsertToDoCheckBoxAction();

    void onEditorInsertTableDialogAction();
    void onEditorInsertTable(int rows, int columns, double width, bool relativeWidth);

private Q_SLOTS:
    // Slots for events from local storage
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            QNLocalizedString errorDescription, QUuid requestId);
    void onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QUuid requestId);

    // Slots for updates from the actual note editor
    void onEditorNoteUpdate(Note note);
    void onEditorNoteUpdateFailed(QNLocalizedString error);

    void onEditorTextBoldStateChanged(bool state);
    void onEditorTextItalicStateChanged(bool state);
    void onEditorTextUnderlineStateChanged(bool state);
    void onEditorTextStrikethroughStateChanged(bool state);
    void onEditorTextAlignLeftStateChanged(bool state);
    void onEditorTextAlignCenterStateChanged(bool state);
    void onEditorTextAlignRightStateChanged(bool state);
    void onEditorTextInsideOrderedListStateChanged(bool state);
    void onEditorTextInsideUnorderedListStateChanged(bool state);
    void onEditorTextInsideTableStateChanged(bool state);
    void onEditorTextFontFamilyChanged(QString fontFamily);
    void onEditorTextFontSizeChanged(int fontSize);
    void onEditorTextInsertTableDialogRequested();

    void onEditorSpellCheckerNotReady();
    void onEditorSpellCheckerReady();

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageWorker);
    void clear();

private:
    Ui::NoteEditorWidget *      m_pUi;
    NoteCache &                 m_noteCache;
    NotebookCache &             m_notebookCache;
    TagCache &                  m_tagCache;

    QScopedPointer<Note>        m_pCurrentNote;
    QScopedPointer<Notebook>    m_pCurrentNotebook;

    QUuid                       m_findCurrentNoteRequestId;
    QUuid                       m_findCurrentNotebookRequestId;

    int                         m_lastFontSizeComboBoxIndex;
    QString                     m_lastFontComboBoxFontFamily;

    int                         m_lastSuggestedFontSize;
    int                         m_lastActualFontSize;

    bool                        m_pendingEditorSpellChecker;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_EDITOR_WIDGET_H

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

#ifndef QUENTIER_WIDGETS_NOTE_EDITOR_WIDGET_H
#define QUENTIER_WIDGETS_NOTE_EDITOR_WIDGET_H

#include "../models/NoteCache.h"
#include "../models/NotebookCache.h"
#include "../models/TagCache.h"
#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Tag.h>
#include <quentier/types/Account.h>
#include <QWidget>
#include <QUuid>
#include <QScopedPointer>
#include <QStringList>
#include <QPointer>
#include <QUndoStack>
#include <QPrinter>

namespace Ui {
class NoteEditorWidget;
}

QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(FileIOProcessorAsync)
QT_FORWARD_DECLARE_CLASS(SpellChecker)

/**
 * @brief The NoteEditorWidget class contains the actual note editor +
 * note title + toolbar with formatting actions + debug html source view +
 * note tags widget
 */
class NoteEditorWidget: public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditorWidget(const Account & account, LocalStorageManagerAsync & localStorageManagerAsync,
                              FileIOProcessorAsync & fileIOProcessorAsync, SpellChecker & spellChecker,
                              NoteCache & noteCache, NotebookCache & notebookCache,
                              TagCache & tagCache, TagModel & tagModel, QUndoStack * pUndoStack,
                              QWidget * parent = Q_NULLPTR);
    virtual ~NoteEditorWidget();

    /**
     * @brief noteLocalUid - getter for the local uid of the editor's note
     * @return the local uid of the note set to the editor, if any; empty string otherwise
     */
    QString noteLocalUid() const;

    /**
     * @return true if the note was created right before opening it in the note editor, false otherwise
     */
    bool isNewNote() const;

    /**
     * @brief setNoteLocalUid - setter for the local uid of the editor's note
     * @param noteLocalUid - the local uid of the note to be set to the editor;
     * the editor finds the note (either within the note cache or within the local storage
     * database) and loads it; setting the empty string removes the note from the editor
     * if it was set there before
     * @param isNewNote - true if the note has been created right before opening it in the note editor, false otherwise.
     * If the note was new and if the note editor widget is closed before the note is edited in any way - title or text
     * or whatever else - it would automatically expunge the empty new note from the local storage. Once the note is saved
     * to the local storage at least once after being loaded into the editor, it is no longer considered new and won't
     * be expunged automatically.
     */
    void setNoteLocalUid(const QString & noteLocalUid, const bool isNewNote = false);

    /**
     * @return true if the widget currently has the full note & notebook object with all the required stuff: note's resources,
     * note's tags, notebook's restrictions (if any) etc; returns false otherwise, if no note local uid was set
     * or if it was set but the query for the full note and/or notebook object from local storage is in progress now
     */
    bool isResolved() const;

    /**
     * @return true if the widget currently has a note loaded and somehow changed and the change has not yet been saved
     * within the local storage; false otherwise
     */
    bool isModified() const;

    /**
     * @return true if the note loaded into the editor within the widget has been modified at least once,
     * false otherwise.
     *
     * This method can be useful if e.g. a new note was created, opened in the editor
     * but then the editor was closed without making a single edit. Such note
     * should be removed from the account in order to prevent its cluttering.
     * This method can tell whether the note has been touched at all and thus tell
     * whether it can be considered for emptiness check and removal.
     */
    bool hasBeenModified() const;

    /**
     * @return title or preview text of the note managed by the editor, if any; empty string otherwise
     */
    QString titleOrPreview() const;

    /**
     * @brief currentNote
     * @return the pointer to the current note loaded into the editor, if any
     */
    const Note * currentNote() const;

    /**
     * @brief isNoteSourceShown
     * @return true if the note source widget showing the note editor's HTML is currently shown,
     * false otherwise
     */
    bool isNoteSourceShown() const;

    /**
     * @brief showNoteSource - shows the note source widget displaying the note editor's HTML
     */
    void showNoteSource();

    /**
     * @brief hideNoteSource - hides the note source widget displaying the note editor's HTML
     */
    void hideNoteSource();

    /**
     * @brief isSpellCheckEnabled
     * @return true if the spell checking is enabled for the note editor, false otherwise
     */
    bool isSpellCheckEnabled() const;

    /**
     * @brief The NoteSaveStatus struct is the namespace wrapper for the enum
     * describing the possible statuses of the attempt to save the changes
     * done to the note within the editor
     */
    struct NoteSaveStatus
    {
        enum type {
            /**
             * Successfully saved the note's contents
             */
            Ok = 0,
            /**
             * Failed to save the note's contents
             */
            Failed,
            /**
             * Failed to finish saving the note's contents in time
             */
            Timeout
        };
    };

    /**
     * @brief checkAndSaveModifiedNote - if the note editor has some note set and
     * it also contains some modifications to the content of the note which are
     * not saved yet, this method attempts to save these synchronously
     * @param errorDescription - contains the textual description of the error
     * if the method wasn't able to save the note
     * @return the result of the attempt to save the note synchronously
     */
    NoteSaveStatus::type checkAndSaveModifiedNote(ErrorString & errorDescription);

    /**
     * @brief isSeparateWindow
     * @return true if the widget has Qt::Window attribute, false otherwise
     */
    bool isSeparateWindow() const;

    /**
     * @brief makeSeparateWindow - sets Qt::Window attribute on NoteEditorWidget +
     * unhides print and export to pdf buttons on the toolbar
     * @return true if the widget did not have Qt::Window attribute before the call,
     * false otherwise
     */
    bool makeSeparateWindow();

    /**
     * @brief makeNonWindow - unset Qt::Window attribute from NoteEditorWidget +
     * hides print and export to pdf buttons on the toolbar
     * @return true if the widget had Qt::Window attribute before the call, false otherwise
     */
    bool makeNonWindow();

    /**
     * @brief setFocusToEditor - sets the focus to the note editor page
     */
    void setFocusToEditor();

    /**
     * @brief printNote - attempts to print the note within the editor (if any)
     * @param errorDescription - the textual description of the error if the note
     * from the editor could not be printed
     * @return true if the note was printed successfully, false otherwise
     */
    bool printNote(ErrorString & errorDescription);

    /**
     * @brief exportNoteToPdf - attempts to export the note within the editor (if any)
     * to a pdf file
     * @param errorDescription - the textual description of the error if the note
     * could not be exported to pdf
     * @return true if the note was exported to pdf successfully, false otherwise
     */
    bool exportNoteToPdf(ErrorString & errorDescription);

    /**
     * @brief exportNoteToEnex - attempts to export the note within the editor (if any)
     * to a enex file
     * @param errorDescription - the textual description of the error if the note
     * could not be exported to enex
     * @return true if the note was exported to pdf successfully, false otherwise
     */
    bool exportNoteToEnex(ErrorString & errorDescription);

    void refreshSpecialIcons();

Q_SIGNALS:
    void notifyError(ErrorString error);

    /**
     * This signal is emitted when note's title or, if note's title has changed
     * (or appeared or disappeared) or, if note has no title and had no title,
     * if note's content has changed in a way affecting the preview text so that
     * it has changed too
     */
    void titleOrPreviewChanged(QString titleOrPreview);

    /**
     * This signal is emitted when full note & notebook objects are found or received
     * by the widget so it can continue its work after setting the note local uid initially
     */
    void resolved();

    /**
     * The signal is emitted after the note has been fully loaded into the editor and is ready for the actual editing
     */
    void noteLoaded();

    /**
     * The note within the editor got either deleted or expunged so this editor needs to be closed
     */
    void invalidated();

    /**
     * The in-app note link was clicked within the editor
     */
    void inAppNoteLinkClicked(QString userId, QString shardId, QString noteGuid);

// private signals
    void updateNote(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

    void noteSavedInLocalStorage();
    void noteSaveInLocalStorageFailed();
    void conversionToNoteFailed();

    void insertInAppNoteLink(const QString & userId, const QString & shardId, const QString & noteGuid, const QString & linkText);

public Q_SLOTS:
    virtual void closeEvent(QCloseEvent * pEvent) Q_DECL_OVERRIDE;

    virtual bool eventFilter(QObject * pWatched, QEvent * pEvent) Q_DECL_OVERRIDE;

    // Slots for toolbar button actions or external actions
    void onEditorTextBoldToggled();
    void onEditorTextItalicToggled();
    void onEditorTextUnderlineToggled();
    void onEditorTextStrikethroughToggled();
    void onEditorTextAlignLeftAction();
    void onEditorTextAlignCenterAction();
    void onEditorTextAlignRightAction();
    void onEditorTextAlignFullAction();
    void onEditorTextAddHorizontalLineAction();
    void onEditorTextIncreaseFontSizeAction();
    void onEditorTextDecreaseFontSizeAction();
    void onEditorTextHighlightAction();
    void onEditorTextIncreaseIndentationAction();
    void onEditorTextDecreaseIndentationAction();
    void onEditorTextInsertUnorderedListAction();
    void onEditorTextInsertOrderedListAction();
    void onEditorTextInsertToDoAction();
    void onEditorTextFormatAsSourceCodeAction();
    void onEditorTextInsertTableDialogRequested();
    void onEditorTextEditHyperlinkAction();
    void onEditorTextCopyHyperlinkAction();
    void onEditorTextRemoveHyperlinkAction();

    void onEditorChooseTextColor(QColor color);
    void onEditorChooseBackgroundColor(QColor color);

    void onEditorSpellCheckStateChanged(int state);
    void onEditorInsertToDoCheckBoxAction();

    void onEditorInsertTableDialogAction();
    void onEditorInsertTable(int rows, int columns, double width, bool relativeWidth);

    void onUndoAction();
    void onRedoAction();

    void onCopyAction();
    void onCutAction();
    void onPasteAction();

    void onSelectAllAction();

    void onSaveNoteAction();

    void onSetUseLimitedFonts(bool useLimitedFonts);

    // Slots for find and replace actions
    void onFindInsideNoteAction();
    void onFindPreviousInsideNoteAction();
    void onReplaceInsideNoteAction();

private Q_SLOTS:
    void onNewTagLineEditReceivedFocusFromWindowSystem();

    void onFontComboBoxFontChanged(const QFont & font);
    void onLimitedFontsComboBoxCurrentIndexChanged(QString fontFamily);

    void onFontSizesComboBoxCurrentIndexChanged(int index);

    // Slots for events from local storage
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            ErrorString errorDescription, QUuid requestId);
    void onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, ErrorString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    /**
     * This slot is called when the editing is still going on, so here we just set the flag that the note title
     * edit has started (so the line edit has "more recent" title that the note itself)
     */
    void onNoteTitleEdited(const QString & noteTitle);

    /**
     * @brief onNoteEditorModified is connected to the note editor's @link noteModified @endlink signal
     */
    void onNoteEditorModified();

    /**
     * This slot is called when the editing of the note title should be considered finished
     */
    void onNoteTitleUpdated();

    // Slots for updates from the actual note editor
    void onEditorNoteUpdate(Note note);
    void onEditorNoteUpdateFailed(ErrorString error);

    void onEditorInAppLinkPasteRequested(QString url, QString userId, QString shardId, QString noteGuid);

    void onEditorTextBoldStateChanged(bool state);
    void onEditorTextItalicStateChanged(bool state);
    void onEditorTextUnderlineStateChanged(bool state);
    void onEditorTextStrikethroughStateChanged(bool state);
    void onEditorTextAlignLeftStateChanged(bool state);
    void onEditorTextAlignCenterStateChanged(bool state);
    void onEditorTextAlignRightStateChanged(bool state);
    void onEditorTextAlignFullStateChanged(bool state);
    void onEditorTextInsideOrderedListStateChanged(bool state);
    void onEditorTextInsideUnorderedListStateChanged(bool state);
    void onEditorTextInsideTableStateChanged(bool state);
    void onEditorTextFontFamilyChanged(QString fontFamily);
    void onEditorTextFontSizeChanged(int fontSize);

    void onEditorSpellCheckerNotReady();
    void onEditorSpellCheckerReady();

    void onEditorHtmlUpdate(QString html);

    // Slots for find & replace widget events
    void onFindAndReplaceWidgetClosed();
    void onTextToFindInsideNoteEdited(const QString & textToFind);
    void onFindNextInsideNote(const QString & textToFind, const bool matchCase);
    void onFindPreviousInsideNote(const QString & textToFind, const bool matchCase);
    void onFindInsideNoteCaseSensitivityChanged(const bool matchCase);
    void onReplaceInsideNote(const QString & textToReplace, const QString & replacementText, const bool matchCase);
    void onReplaceAllInsideNote(const QString & textToReplace, const QString & replacementText, const bool matchCase);

    // Helper slot called from QTimer::singleShot
    void updateNoteInLocalStorage();

    // Slots for print/export buttons
    void onPrintNoteButtonPressed();
    void onExportNoteToPdfButtonPressed();
    void onExportNoteToEnexButtonPressed();

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void clear();

    void onCurrentNoteFound(const Note & note);

    void setupSpecialIcons();
    void setupFontsComboBox();
    void setupLimitedFontsComboBox(const QString & startupFont = QString());

    void updateNoteSourceView(const QString & html);

    void setNoteAndNotebook(const Note & note, const Notebook & notebook);

    QString blankPageHtml() const;
    void setupBlankEditor();

    bool checkNoteTitle(const QString & title, ErrorString & errorDescription);

    void removeSurrondingApostrophes(QString & str) const;

private:
    Ui::NoteEditorWidget *      m_pUi;
    NoteCache &                 m_noteCache;
    NotebookCache &             m_notebookCache;
    TagCache &                  m_tagCache;

    FileIOProcessorAsync &      m_fileIOProcessorAsync;
    SpellChecker &              m_spellChecker;

    QStringListModel *          m_pLimitedFontsListModel;

    // This data piece separate from m_pCurrentNote is needed in order to handle
    // the cases when the note is being loaded from the local storage while
    // someone asks which note local uid the widget handles
    QString                     m_noteLocalUid;

    QScopedPointer<Note>        m_pCurrentNote;
    QScopedPointer<Notebook>    m_pCurrentNotebook;

    QString                     m_lastNoteTitleOrPreviewText;

    Account                     m_currentAccount;
    QPointer<QUndoStack>        m_pUndoStack;

    QTimer *                    m_pConvertToNoteDeadlineTimer;

    QUuid                       m_findCurrentNoteRequestId;
    QUuid                       m_findCurrentNotebookRequestId;
    QSet<QUuid>                 m_updateNoteRequestIds;

    class NoteLinkInfo: public Printable
    {
    public:
        QString     m_userId;
        QString     m_shardId;
        QString     m_noteGuid;

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    QHash<QUuid, NoteLinkInfo>  m_noteLinkInfoByFindNoteRequestIds;

    int                         m_lastFontSizeComboBoxIndex;
    QString                     m_lastFontComboBoxFontFamily;

    QString                     m_lastNoteEditorHtml;

    StringUtils                 m_stringUtils;

    int                         m_lastSuggestedFontSize;
    int                         m_lastActualFontSize;

    bool                        m_processingFontFamilyUpdateFromEditor;

    bool                        m_pendingEditorSpellChecker;
    bool                        m_currentNoteWasExpunged;

    bool                        m_noteHasBeenModified;

    bool                        m_noteTitleIsEdited;
    bool                        m_noteTitleHasBeenEdited;

    bool                        m_isNewNote;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_EDITOR_WIDGET_H

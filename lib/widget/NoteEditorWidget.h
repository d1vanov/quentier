/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#pragma once

#include <lib/model/note/NoteCache.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/tag/TagCache.h>

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/StringUtils.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>

#include <QPointer>
#include <QPrinter>
#include <QStringList>
#include <QUndoStack>
#include <QWidget>

#include <optional>

namespace Ui {

class NoteEditorWidget;

} // namespace Ui

class QDebug;
class QStringListModel;
class QThread;
class QTimer;

namespace quentier {

class SpellChecker;
class TagModel;

/**
 * @brief The NoteEditorWidget class contains the actual note editor +
 * note title + toolbar with formatting actions + debug html source view +
 * note tags widget
 */
class NoteEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditorWidget(
        Account account, local_storage::ILocalStoragePtr localStorage,
        SpellChecker & spellChecker, QThread * backgroundJobsThread,
        NoteCache & noteCache, NotebookCache & notebookCache,
        TagCache & tagCache, TagModel & tagModel, QUndoStack * undoStack,
        QWidget * parent = nullptr);

    ~NoteEditorWidget() override;

    /**
     * @return                  Local id of the note set to the editor,
     *                          if any; empty string otherwise
     */
    [[nodiscard]] QString noteLocalId() const;

    /**
     * @return                  True if the note was created right before
     *                          opening it in the note editor, false otherwise
     */
    [[nodiscard]] bool isNewNote() const noexcept;

    /**
     * @param noteLocalId       Local id of the note to be set to the editor;
     *                          the editor finds the note (either in the
     *                          note cache or in local storage) and loads it;
     *                          setting empty string removes the note from
     *                          the editor if it was set there before
     * @param isNewNote         True if the note has been created right before
     *                          opening it in the note editor, false otherwise.
     *                          If the note was new and if the note editor
     *                          widget is closed before the note is edited in
     *                          any way - title or text or whatever else - it
     *                          would automatically expunge the empty new note
     *                          from the local storage. Once the note is saved
     *                          to the local storage at least once after being
     *                          loaded into the editor, it is no longer
     *                          considered new and won't be expunged
     *                          automatically.
     */
    void setNoteLocalId(
        const QString & noteLocalId, const bool isNewNote = false);

    /**
     * @return                  True if the widget currently has the full note &
     *                          notebook object with all the required stuff:
     *                          note's resources, note's tags, notebook's
     *                          restrictions (if any) etc; returns false
     *                          otherwise, if no note local uid was set or if it
     *                          was set but the query for the full note and/or
     *                          notebook object from the local storage is in
     *                          progress now
     */
    [[nodiscard]] bool isResolved() const noexcept;

    /**
     * @return                  True if the widget currently has a note loaded
     *                          and somehow changed and the change has not yet
     *                          been saved within the local storage; false
     *                          otherwise
     */
    [[nodiscard]] bool isModified() const noexcept;

    /**
     * @return                  True if the note loaded into the editor within
     *                          the widget has been modified at least once,
     *                          false otherwise.
     *
     * This method can be useful if e.g. a new note was created, opened in
     * the editor but then the editor was closed without making a single edit.
     * Such note is useless and thus should be removed from the account in order
     * to prevent its cluttering. This method can tell whether the note has been
     * touched at all and thus tell whether it can be considered for emptiness
     * check and removal.
     */
    [[nodiscard]] bool hasBeenModified() const noexcept;

    /**
     * @return the number of milliseconds since the last user's interaction
     * with the note editor or -1 if there was no interaction or if no note
     * is loaded at the moment
     */
    [[nodiscard]] qint64 idleTime() const noexcept;

    /**
     * @return                  Title or preview text of the note managed by
     *                          the editor, if any; empty string otherwise
     */
    [[nodiscard]] QString titleOrPreview() const;

    /**
     * @brief currentNote
     * @return                  Current note loaded into the editor, if any
     */
    [[nodiscard]] const std::optional<qevercloud::Note> & currentNote()
        const noexcept;

    /**
     * @brief isNoteSourceShown
     * @return                  True if the note source widget showing the note
     *                          editor's HTML is currently shown, false
     *                          otherwise
     */
    [[nodiscard]] bool isNoteSourceShown() const;

    /**
     * @brief showNoteSource - shows the note source widget displaying the note
     * editor's HTML
     */
    void showNoteSource();

    /**
     * @brief hideNoteSource - hides the note source widget displaying the note
     * editor's HTML
     */
    void hideNoteSource();

    /**
     * @brief isSpellCheckEnabled
     * @return                  True if the spell checking is enabled for
     *                          the note editor, false otherwise
     */
    [[nodiscard]] bool isSpellCheckEnabled() const;

    /**
     * @brief The NoteSaveStatus enum describes possible statuses of the attempt
     * to save changes done to the note within the editor.
     */
    enum class NoteSaveStatus
    {
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

    friend QDebug & operator<<(QDebug & dbg, NoteSaveStatus status);

    /**
     * @brief checkAndSaveModifiedNote - if the note editor has some note set
     * and it also contains some modifications to the content of the note which
     * are not saved yet, this method attempts to save these synchronously
     *
     * @param errorDescription      Contains the textual description of
     *                              the error if the method wasn't able to save
     *                              the note
     * @return                      The result of the attempt to save the note
     *                              synchronously
     */
    [[nodiscard]] NoteSaveStatus checkAndSaveModifiedNote(
        ErrorString & errorDescription);

    /**
     * @brief isSeparateWindow
     * @return                      True if the widget has Qt::Window attribute,
     *                              false otherwise
     */
    [[nodiscard]] bool isSeparateWindow() const;

    /**
     * @brief makeSeparateWindow - sets Qt::Window attribute on NoteEditorWidget
     * + unhides print and export to pdf buttons on the toolbar
     *
     * @return                      True if the widget did not have Qt::Window
     *                              attribute before the call, false otherwise
     */
    [[nodiscard]] bool makeSeparateWindow();

    /**
     * @brief makeNonWindow - unset Qt::Window attribute from NoteEditorWidget +
     * hides print and export to pdf buttons on the toolbar
     *
     * @return                      True if the widget had Qt::Window attribute
     *                              before the call, false otherwise
     */
    [[nodiscard]] bool makeNonWindow();

    /**
     * @brief setFocusToEditor - sets the focus to the note editor page
     */
    void setFocusToEditor();

    /**
     * @brief printNote - attempts to print the note within the editor (if any)
     * @param errorDescription      The textual description of the error if
     *                              the note from the editor could not be
     *                              printed
     * @return                      True if the note was printed successfully,
     *                              false otherwise
     */
    [[nodiscard]] bool printNote(ErrorString & errorDescription);

    /**
     * @brief exportNoteToPdf - attempts to export the note within the editor
     * (if any) to a pdf file
     *
     * @param errorDescription      The textual description of the error if
     *                              the note could not be exported to pdf
     * @return true if the note was exported to pdf successfully, false
     * otherwise
     */
    [[nodiscard]] bool exportNoteToPdf(ErrorString & errorDescription);

    /**
     * @brief exportNoteToEnex - attempts to export the note within the editor
     * (if any) to a enex file
     *
     * @param errorDescription      The textual description of the error if
     *                              the note could not be exported to enex
     * @return                      True if the note was exported to pdf
     *                              successfully, false otherwise
     */
    [[nodiscard]] bool exportNoteToEnex(ErrorString & errorDescription);

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
     * This signal is emitted when full note & notebook objects are found or
     * received by the widget so it can continue its work after setting the note
     * local uid initially
     */
    void resolved();

    /**
     * The signal is emitted after the note has been fully loaded into
     * the editor and is ready for the actual editing
     */
    void noteLoaded();

    /**
     * The note within the editor got either deleted or expunged so this editor
     * needs to be closed
     */
    void invalidated();

    /**
     * The in-app note link was clicked within the editor
     */
    void inAppNoteLinkClicked(
        QString userId, QString shardId, QString noteGuid);

    // private signals
    void noteSavedInLocalStorage();
    void noteSaveInLocalStorageFailed();
    void conversionToNoteFailed();

    void insertInAppNoteLink(
        const QString & userId, const QString & shardId,
        const QString & noteGuid, const QString & linkText);

public:
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dragMoveEvent(QDragMoveEvent * event) override;
    void dropEvent(QDropEvent * event) override;
    void closeEvent(QCloseEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;

public Q_SLOTS:
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

    void onEditorInsertTable(
        int rows, int columns, double width, bool relativeWidth);

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

    void onNoteEditorFontColorChanged(const QColor & color);
    void onNoteEditorBackgroundColorChanged(const QColor & color);
    void onNoteEditorHighlightColorChanged(const QColor & color);
    void onNoteEditorHighlightedTextColorChanged(const QColor & color);
    void onNoteEditorColorsReset();

private Q_SLOTS:
    void onNoteTagsListChanged(qevercloud::Note note);
    void onNewTagLineEditReceivedFocusFromWindowSystem();

    void onFontComboBoxFontChanged(const QFont & font);
    void onLimitedFontsComboBoxCurrentIndexChanged(int fontFamilyIndex);

    void onFontSizesComboBoxCurrentIndexChanged(int index);

    void onNoteSavedToLocalStorage(QString noteLocalId);

    void onFailedToSaveNoteToLocalStorage(
        ErrorString errorDescription, QString noteLocalId);

    /**
     * This slot is called when the editing is still going on, so here we just
     * set the flag that the note title edit has started (so the line edit has
     * "more recent" title that the note itself)
     */
    void onNoteTitleEdited(const QString & noteTitle);

    /**
     * @brief onNoteEditorModified is connected to the note editor's
     * noteModified signal
     */
    void onNoteEditorModified();

    /**
     * This slot is called when the editing of the note title should be
     * considered finished
     */
    void onNoteTitleUpdated();

    // Slots for updates from the actual note editor
    void onEditorNoteUpdate(qevercloud::Note note);
    void onEditorNoteUpdateFailed(const ErrorString & error);

    void onEditorInAppLinkPasteRequested(
        QString url, QString userId, QString shardId, QString noteGuid);

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

    void onFoundNoteAndNotebookInLocalStorage(
        const qevercloud::Note & note, const qevercloud::Notebook & notebook);

    void onNoteNotFoundInLocalStorage(const QString & noteLocalId);

    // Slots for find & replace widget events
    void onFindAndReplaceWidgetClosed();
    void onTextToFindInsideNoteEdited(const QString & textToFind);
    void onFindNextInsideNote(const QString & textToFind, bool matchCase);

    void onFindPreviousInsideNote(
        const QString & textToFind, bool matchCase);

    void onFindInsideNoteCaseSensitivityChanged(bool matchCase);

    void onReplaceInsideNote(
        const QString & textToReplace, const QString & replacementText,
        bool matchCase);

    void onReplaceAllInsideNote(
        const QString & textToReplace, const QString & replacementText,
        bool matchCase);

    void updateNoteInLocalStorage();

    // Slots for local storage events
    void onResourcePut(const qevercloud::Resource & resource);
    void onResourceMetadataPut(const qevercloud::Resource & resource);
    void onResourceExpunged(const QString & resourceLocalId);

    void onNotebookPut(const qevercloud::Notebook & notebook);
    void onNotebookExpunged(const QString & notebookLocalId);

    // Slots for print/export buttons
    void onPrintNoteButtonPressed();
    void onExportNoteToPdfButtonPressed();
    void onExportNoteToEnexButtonPressed();

private:
    void createConnections();
    void clear();

    void onNotePut(
        const qevercloud::Note & note, bool resourcesUpdated, bool tagsUpdated);

    void onNoteExpunged(const QString & noteLocalId);

    void setupNoteEditorColors();
    void onNoteEditorColorsUpdate();

    void setupSpecialIcons();

    void setupFontsComboBox();
    void setupLimitedFontsComboBox(const QString & startupFont = {});
    void setupFontSizesComboBox();
    void setupFontSizesForFont(const QFont & font);
    bool useLimitedSetOfFonts() const;
    void setupNoteEditorDefaultFont();

    void updateNoteSourceView(const QString & html);

    void setNoteAndNotebook(
        const qevercloud::Note & note, const qevercloud::Notebook & notebook);

    [[nodiscard]] QString blankPageHtml() const;
    void setupBlankEditor();

    [[nodiscard]] bool checkNoteTitle(
        const QString & title, ErrorString & errorDescription);

    void removeSurrondingApostrophes(QString & str) const;

private:
    const local_storage::ILocalStoragePtr m_localStorage;

    Ui::NoteEditorWidget * m_ui;
    NoteCache & m_noteCache;
    NotebookCache & m_notebookCache;
    TagCache & m_tagCache;

    QStringListModel * m_limitedFontsListModel = nullptr;

    // This data piece separate from m_currentNote is needed in order to handle
    // the cases when the note is being loaded from the local storage while
    // someone asks which note local uid the widget handles
    QString m_noteLocalId;

    std::optional<qevercloud::Note> m_currentNote;
    std::optional<qevercloud::Notebook> m_currentNotebook;

    bool m_pendingFindingCurrentNotebook = false;

    QString m_lastNoteTitleOrPreviewText;

    Account m_currentAccount;
    QPointer<QUndoStack> m_undoStack;

    QTimer * m_convertToNoteDeadlineTimer = nullptr;

    class NoteLinkInfo : public Printable
    {
    public:
        QString m_userId;
        QString m_shardId;
        QString m_noteGuid;

        QTextStream & print(QTextStream & strm) const override;
    };

    int m_lastFontSizeComboBoxIndex = -1;
    QString m_lastFontComboBoxFontFamily;

    QString m_lastNoteEditorHtml;

    StringUtils m_stringUtils;

    int m_lastSuggestedFontSize = -1;
    int m_lastActualFontSize = -1;

    bool m_pendingEditorSpellChecker = false;
    bool m_currentNoteWasExpunged = false;

    bool m_noteHasBeenModified = false;

    bool m_noteTitleIsEdited = false;
    bool m_noteTitleHasBeenEdited = false;

    bool m_isNewNote = false;
};

} // namespace quentier

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

#include "NoteEditorWidget.h"
#include "../SettingsNames.h"
#include "../DefaultSettings.h"
#include "../models/TagModel.h"
#include "../dialogs/EnexExportDialog.h"
#include "../delegates/LimitedFontsDelegate.h"

// Doh, Qt Designer's inability to work with namespaces in the expected way
// is deeply disappointing
#include "NoteTagsWidget.h"
#include "NewListItemLineEdit.h"
#include "FindAndReplaceWidget.h"
#include "../BasicXMLSyntaxHighlighter.h"
#include "../insert-table-tool-button/InsertTableToolButton.h"
#include "../insert-table-tool-button/TableSettingsDialog.h"
#include "../color-picker-tool-button/ColorPickerToolButton.h"
#include <quentier/note_editor/NoteEditor.h>
using quentier::FindAndReplaceWidget;
using quentier::NoteEditor;
using quentier::NoteTagsWidget;
#include "ui_NoteEditorWidget.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Resource.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/FileIOProcessorAsync.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/note_editor/SpellChecker.h>
#include <QDateTime>
#include <QFontDatabase>
#include <QScopedPointer>
#include <QColor>
#include <QPalette>
#include <QTimer>
#include <QToolTip>
#include <QPrintDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringListModel>

#define CHECK_NOTE_SET() \
    if (Q_UNLIKELY(m_pCurrentNote.isNull()) { \
        Q_EMIT notifyError(QT_TRANSLATE_NOOP("NoteEditorWidget", "No note is set to the editor")); \
        return; \
    }

namespace quentier {

NoteEditorWidget::NoteEditorWidget(const Account & account, LocalStorageManagerAsync & localStorageManagerAsync,
                                   FileIOProcessorAsync & fileIOProcessorAsync, SpellChecker & spellChecker,
                                   NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
                                   TagModel & tagModel, QUndoStack * pUndoStack, QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::NoteEditorWidget),
    m_noteCache(noteCache),
    m_notebookCache(notebookCache),
    m_tagCache(tagCache),
    m_fileIOProcessorAsync(fileIOProcessorAsync),
    m_spellChecker(spellChecker),
    m_pLimitedFontsListModel(Q_NULLPTR),
    m_noteLocalUid(),
    m_pCurrentNote(),
    m_pCurrentNotebook(),
    m_lastNoteTitleOrPreviewText(),
    m_currentAccount(account),
    m_pUndoStack(pUndoStack),
    m_pConvertToNoteDeadlineTimer(Q_NULLPTR),
    m_findCurrentNoteRequestId(),
    m_findCurrentNotebookRequestId(),
    m_updateNoteRequestIds(),
    m_noteLinkInfoByFindNoteRequestIds(),
    m_lastFontSizeComboBoxIndex(-1),
    m_lastFontComboBoxFontFamily(),
    m_lastNoteEditorHtml(),
    m_stringUtils(),
    m_lastSuggestedFontSize(-1),
    m_lastActualFontSize(-1),
    m_processingFontFamilyUpdateFromEditor(false),
    m_pendingEditorSpellChecker(false),
    m_currentNoteWasExpunged(false),
    m_noteHasBeenModified(false),
    m_noteTitleIsEdited(false),
    m_noteTitleHasBeenEdited(false),
    m_isNewNote(false)
{
    m_pUi->setupUi(this);

    setupSpecialIcons();
    setupFontsComboBox();
    setupFontSizesComboBox();

    // Originally the NoteEditorWidget is not a window, so hide these two buttons,
    // they are for the separate-window mode only
    m_pUi->printNotePushButton->setHidden(true);
    m_pUi->exportNoteToPdfPushButton->setHidden(true);

    m_pUi->noteEditor->initialize(m_fileIOProcessorAsync, m_spellChecker, m_currentAccount);
    m_pUi->saveNotePushButton->setEnabled(false);

    m_pUi->noteNameLineEdit->installEventFilter(this);

    m_pUi->noteEditor->setUndoStack(m_pUndoStack.data());

    setupBlankEditor();

    BasicXMLSyntaxHighlighter * highlighter = new BasicXMLSyntaxHighlighter(m_pUi->noteSourceView->document());
    Q_UNUSED(highlighter);

    m_pUi->tagNameLabelsContainer->setTagModel(&tagModel);
    m_pUi->tagNameLabelsContainer->setLocalStorageManagerThreadWorker(localStorageManagerAsync);
    createConnections(localStorageManagerAsync);

    QWidget::setAttribute(Qt::WA_DeleteOnClose, /* on = */ true);
}

NoteEditorWidget::~NoteEditorWidget()
{}

QString NoteEditorWidget::noteLocalUid() const
{
    return m_noteLocalUid;
}

bool NoteEditorWidget::isNewNote() const
{
    return m_isNewNote;
}

void NoteEditorWidget::setNoteLocalUid(const QString & noteLocalUid, const bool isNewNote)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setNoteLocalUid: ") << noteLocalUid
            << QStringLiteral(", is new note = ") << (isNewNote ? QStringLiteral("true") : QStringLiteral("false")));

    m_noteLocalUid = noteLocalUid;

    if (!m_pCurrentNote.isNull() && (m_pCurrentNote->localUid() == noteLocalUid)) {
        QNDEBUG(QStringLiteral("This note is already set to the editor, nothing to do"));
        return;
    }

    clear();

    if (noteLocalUid.isEmpty()) {
        setupBlankEditor();
        return;
    }

    m_isNewNote = isNewNote;

    const Note * pCachedNote = m_noteCache.get(noteLocalUid);

    // The cache might contain the note without resource binary data, need to check for this
    bool hasMissingResources = false;
    if (Q_LIKELY(pCachedNote))
    {
        QList<Resource> resources = pCachedNote->resources();
        if (!resources.isEmpty())
        {
            for(int i = 0, size = resources.size(); i < size; ++i)
            {
                const Resource & resource = resources[i];
                if (resource.hasDataHash() && !resource.hasDataBody()) {
                    hasMissingResources = true;
                    break;
                }
            }
        }
    }

    if (Q_UNLIKELY(!pCachedNote || hasMissingResources))
    {
        m_findCurrentNoteRequestId = QUuid::createUuid();
        Note dummy;
        dummy.setLocalUid(noteLocalUid);
        QNTRACE(QStringLiteral("Emitting the request to find the current note: local uid = ") << noteLocalUid
                << QStringLiteral(", request id = ") << m_findCurrentNoteRequestId);
        Q_EMIT findNote(dummy, /* with resource binary data = */ true, m_findCurrentNoteRequestId);
        return;
    }

    QNTRACE(QStringLiteral("Found the cached note"));

    if (Q_UNLIKELY(!pCachedNote->hasNotebookLocalUid() && !pCachedNote->hasNotebookGuid())) {
        ErrorString error(QT_TR_NOOP("Can't set the note to the editor: the note has no linkage to any notebook"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    m_pCurrentNote.reset(new Note(*pCachedNote));
    QNTRACE(QStringLiteral("Note: ") << *m_pCurrentNote);

    const Notebook * pCachedNotebook = Q_NULLPTR;
    if (m_pCurrentNote->hasNotebookLocalUid()) {
        pCachedNotebook = m_notebookCache.get(m_pCurrentNote->notebookLocalUid());
    }

    if (Q_UNLIKELY(!pCachedNotebook))
    {
        m_findCurrentNotebookRequestId = QUuid::createUuid();
        Notebook dummy;
        if (m_pCurrentNote->hasNotebookLocalUid()) {
            dummy.setLocalUid(m_pCurrentNote->notebookLocalUid());
        }
        else {
            dummy.setLocalUid(QString());
            dummy.setGuid(m_pCurrentNote->notebookGuid());
        }

        QNTRACE(QStringLiteral("Emitting the request to find the notebook for note: ") << dummy
                << QStringLiteral("\nRequest id = ") << m_findCurrentNotebookRequestId);
        Q_EMIT findNotebook(dummy, m_findCurrentNotebookRequestId);
        return;
    }

    m_pCurrentNotebook.reset(new Notebook(*pCachedNotebook));
    QNTRACE(QStringLiteral("Notebook: ") << *m_pCurrentNotebook);

    setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);

    QNTRACE(QStringLiteral("Emitting resolved signal, note local uid = ") << m_noteLocalUid);
    Q_EMIT resolved();
}

bool NoteEditorWidget::isResolved() const
{
    return !m_pCurrentNote.isNull() && !m_pCurrentNotebook.isNull();
}

bool NoteEditorWidget::isModified() const
{
    return !m_pCurrentNote.isNull() &&
            (m_pUi->noteEditor->isModified() || m_noteTitleIsEdited);
}

bool NoteEditorWidget::hasBeenModified() const
{
    return m_noteHasBeenModified || m_noteTitleHasBeenEdited;
}

QString NoteEditorWidget::titleOrPreview() const
{
    if (Q_UNLIKELY(m_pCurrentNote.isNull())) {
        return QString();
    }

    if (m_pCurrentNote->hasTitle()) {
        return m_pCurrentNote->title();
    }

    if (m_pCurrentNote->hasContent()) {
        QString previewText = m_pCurrentNote->plainText();
        previewText.truncate(140);
        return previewText;
    }

    return QString();
}

const Note * NoteEditorWidget::currentNote() const
{
    return m_pCurrentNote.data();
}

bool NoteEditorWidget::isNoteSourceShown() const
{
    return m_pUi->noteSourceView->isVisible();
}

void NoteEditorWidget::showNoteSource()
{
    updateNoteSourceView(m_lastNoteEditorHtml);
    m_pUi->noteSourceView->setHidden(false);
}

void NoteEditorWidget::hideNoteSource()
{
    m_pUi->noteSourceView->setHidden(true);
}

bool NoteEditorWidget::isSpellCheckEnabled() const
{
    return m_pUi->noteEditor->spellCheckEnabled();
}

NoteEditorWidget::NoteSaveStatus::type NoteEditorWidget::checkAndSaveModifiedNote(ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::checkAndSaveModifiedNote"));

    if (m_pCurrentNote.isNull()) {
        QNDEBUG(QStringLiteral("No note is set to the editor"));
        return NoteSaveStatus::Ok;
    }

    if (m_pCurrentNote->hasDeletionTimestamp()) {
        QNDEBUG(QStringLiteral("The note is deleted which means it just got deleted and the editor is closing => "
                               "there is no need to save whatever is left in the editor for this note"));
        return NoteSaveStatus::Ok;
    }

    bool noteContentModified = m_pUi->noteEditor->isModified();

    if (!m_noteTitleIsEdited && !noteContentModified) {
        QNDEBUG(QStringLiteral("Note is not modified, nothing to save"));
        return NoteSaveStatus::Ok;
    }

    bool noteTitleUpdated = false;

    if (m_noteTitleIsEdited)
    {
        QString noteTitle = m_pUi->noteNameLineEdit->text().trimmed();
        m_stringUtils.removeNewlines(noteTitle);

        if (noteTitle.isEmpty() && m_pCurrentNote->hasTitle())
        {
            m_pCurrentNote->setTitle(QString());  // That should erase the title from the note
            noteTitleUpdated = true;
        }
        else if (!noteTitle.isEmpty() && (!m_pCurrentNote->hasTitle() || (m_pCurrentNote->title() != noteTitle)))
        {
            ErrorString error;
            if (checkNoteTitle(noteTitle, error)) {
                m_pCurrentNote->setTitle(noteTitle);
                noteTitleUpdated = true;
            }
            else {
                QNDEBUG(QStringLiteral("Couldn't save the note title: ") << error);
            }
        }
    }

    if (noteTitleUpdated) {
        // NOTE: indicating early that the note's title has been changed manually
        // and not generated automatically
        qevercloud::NoteAttributes & attributes = m_pCurrentNote->noteAttributes();
        attributes.noteTitleQuality.clear();
    }

    if (noteContentModified || noteTitleUpdated)
    {
        ApplicationSettings appSettings;
        appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
        QVariant editorConvertToNoteTimeoutData = appSettings.value(CONVERT_TO_NOTE_TIMEOUT_SETTINGS_KEY);
        appSettings.endGroup();

        bool conversionResult = false;
        int editorConvertToNoteTimeout = editorConvertToNoteTimeoutData.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(QStringLiteral("Can't read the timeout for note editor to note conversion from the application settings, "
                                   "fallback to the default value of ") << DEFAULT_EDITOR_CONVERT_TO_NOTE_TIMEOUT
                    << QStringLiteral(" milliseconds"));
            editorConvertToNoteTimeout = DEFAULT_EDITOR_CONVERT_TO_NOTE_TIMEOUT;
        }
        else {
            editorConvertToNoteTimeout = std::max(editorConvertToNoteTimeout, 100);
        }

        if (m_pConvertToNoteDeadlineTimer) {
            m_pConvertToNoteDeadlineTimer->deleteLater();
        }

        m_pConvertToNoteDeadlineTimer = new QTimer(this);
        m_pConvertToNoteDeadlineTimer->setSingleShot(true);

        EventLoopWithExitStatus eventLoop;
        QObject::connect(m_pConvertToNoteDeadlineTimer, QNSIGNAL(QTimer,timeout),
                         &eventLoop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(this, QNSIGNAL(NoteEditorWidget,noteSavedInLocalStorage),
                         &eventLoop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
        QObject::connect(this, QNSIGNAL(NoteEditorWidget,noteSaveInLocalStorageFailed),
                         &eventLoop, QNSLOT(EventLoopWithExitStatus,exitAsFailure));
        QObject::connect(this, QNSIGNAL(NoteEditorWidget,conversionToNoteFailed),
                         &eventLoop, QNSLOT(EventLoopWithExitStatus,exitAsFailure));

        m_pConvertToNoteDeadlineTimer->start(editorConvertToNoteTimeout);

        if (noteContentModified) {
            QTimer::singleShot(0, m_pUi->noteEditor, SLOT(convertToNote()));
        }
        else {
            QTimer::singleShot(0, this, SLOT(updateNoteInLocalStorage()));
        }

        int result = eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

        if (result == EventLoopWithExitStatus::ExitStatus::Failure) {
            errorDescription.setBase(QT_TR_NOOP("Failed to convert the editor contents to note"));
            QNWARNING(errorDescription);
            return NoteSaveStatus::Failed;
        }
        else if (result == EventLoopWithExitStatus::ExitStatus::Timeout) {
            errorDescription.setBase(QT_TR_NOOP("The conversion of note editor contents "
                                                "to note failed to finish in time"));
            QNWARNING(errorDescription);
            return NoteSaveStatus::Timeout;
        }
    }

    return NoteSaveStatus::Ok;
}

bool NoteEditorWidget::isSeparateWindow() const
{
    Qt::WindowFlags flags = windowFlags();
    return flags.testFlag(Qt::Window);
}

bool NoteEditorWidget::makeSeparateWindow()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::makeSeparateWindow"));

    if (isSeparateWindow()) {
        return false;
    }

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::Window;

    QWidget::setWindowFlags(flags);

    m_pUi->printNotePushButton->setHidden(false);
    m_pUi->exportNoteToPdfPushButton->setHidden(false);
    return true;
}

bool NoteEditorWidget::makeNonWindow()
{
    if (!isSeparateWindow()) {
        return false;
    }

    Qt::WindowFlags flags = windowFlags();
    flags &= (~Qt::Window);

    QWidget::setWindowFlags(flags);

    m_pUi->printNotePushButton->setHidden(true);
    m_pUi->exportNoteToPdfPushButton->setHidden(true);
    return true;
}

void NoteEditorWidget::setFocusToEditor()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setFocusToEditor"));
    m_pUi->noteEditor->setFocus();
}

bool NoteEditorWidget::printNote(ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::printNote"));

    if (Q_UNLIKELY(m_pCurrentNote.isNull())) {
        errorDescription.setBase(QT_TR_NOOP("Can't print note: no note is set to the editor"));
        QNDEBUG(errorDescription);
        return false;
    }

    QPrinter printer;
    QScopedPointer<QPrintDialog> pPrintDialog(new QPrintDialog(&printer, this));
    pPrintDialog->setWindowModality(Qt::WindowModal);

    QAbstractPrintDialog::PrintDialogOptions options;
    options |= QAbstractPrintDialog::PrintToFile;
    options |= QAbstractPrintDialog::PrintCollateCopies;

    pPrintDialog->setOptions(options);

    if (pPrintDialog->exec() == QDialog::Accepted) {
        return m_pUi->noteEditor->print(printer, errorDescription);
    }

    QNTRACE(QStringLiteral("Note printing has been cancelled"));
    return false;
}

bool NoteEditorWidget::exportNoteToPdf(ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::exportNoteToPdf"));

    if (Q_UNLIKELY(m_pCurrentNote.isNull())) {
        errorDescription.setBase(QT_TR_NOOP("Can't export note to pdf: no note within the note editor widget"));
        return false;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QString lastExportNoteToPdfPath = appSettings.value(LAST_EXPORT_NOTE_TO_PDF_PATH_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastExportNoteToPdfPath.isEmpty()) {
        lastExportNoteToPdfPath = documentsPath();
    }

    QScopedPointer<QFileDialog> pFileDialog(new QFileDialog(this,
                                                            tr("Please select the output pdf file"),
                                                            lastExportNoteToPdfPath));
    pFileDialog->setWindowModality(Qt::WindowModal);
    pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    pFileDialog->setFileMode(QFileDialog::AnyFile);
    pFileDialog->setDefaultSuffix(QStringLiteral("pdf"));

    QString suggestedFileName;
    if (m_pCurrentNote->hasTitle()) {
        suggestedFileName = m_pCurrentNote->title();
    }
    else if (m_pCurrentNote->hasContent()) {
        suggestedFileName = m_pCurrentNote->plainText().simplified();
        suggestedFileName.truncate(30);
    }

    if (!suggestedFileName.isEmpty()) {
        suggestedFileName += QStringLiteral(".pdf");
        pFileDialog->selectFile(suggestedFileName);
    }

    if (pFileDialog->exec() == QDialog::Accepted)
    {
        QStringList selectedFiles = pFileDialog->selectedFiles();
        int numSelectedFiles = selectedFiles.size();

        if (numSelectedFiles == 0) {
            errorDescription.setBase(QT_TR_NOOP("No pdf file was selected to export the note into"));
            return false;
        }

        if (numSelectedFiles > 1) {
            errorDescription.setBase(QT_TR_NOOP("More than one file were selected as output pdf files"));
            errorDescription.details() = selectedFiles.join(QStringLiteral(", "));
            return false;
        }

        QFileInfo selectedFileInfo(selectedFiles[0]);
        if (selectedFileInfo.exists())
        {
            if (Q_UNLIKELY(!selectedFileInfo.isFile())) {
                errorDescription.setBase(QT_TR_NOOP("No file to save the pdf into was selected"));
                return false;
            }

            if (Q_UNLIKELY(!selectedFileInfo.isWritable())) {
                errorDescription.setBase(QT_TR_NOOP("The selected file already exists and it is not writable"));
                errorDescription.details() = selectedFiles[0];
                return false;
            }

            int confirmOverwrite = questionMessageBox(this, tr("Overwrite existing file"),
                                                      tr("Confirm the choice to overwrite the existing file"),
                                                      tr("The selected pdf file already exists. Are you sure you want to overwrite this file?"));
            if (confirmOverwrite != QMessageBox::Ok) {
                QNDEBUG(QStringLiteral("Cancelled the export to pdf"));
                return false;
            }
        }

        lastExportNoteToPdfPath = selectedFileInfo.absolutePath();
        if (!lastExportNoteToPdfPath.isEmpty()) {
            appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
            appSettings.setValue(LAST_EXPORT_NOTE_TO_PDF_PATH_SETTINGS_KEY, lastExportNoteToPdfPath);
            appSettings.endGroup();
        }

        return m_pUi->noteEditor->exportToPdf(selectedFiles[0], errorDescription);
    }

    QNTRACE(QStringLiteral("Exporting the note to pdf has been cancelled"));
    return false;
}

bool NoteEditorWidget::exportNoteToEnex(ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::exportNoteToEnex"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QString lastExportNoteToEnexPath = appSettings.value(LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastExportNoteToEnexPath.isEmpty()) {
        lastExportNoteToEnexPath = documentsPath();
    }

    QScopedPointer<EnexExportDialog> pExportEnexDialog(new EnexExportDialog(m_currentAccount,
                                                                            this, titleOrPreview()));
    pExportEnexDialog->setWindowModality(Qt::WindowModal);

    if (pExportEnexDialog->exec() == QDialog::Accepted)
    {
        QNDEBUG(QStringLiteral("Confirmed ENEX export"));

        QString enexFilePath = pExportEnexDialog->exportEnexFilePath();

        QFileInfo enexFileInfo(enexFilePath);
        if (enexFileInfo.exists())
        {
            if (!enexFileInfo.isWritable()) {
                errorDescription.setBase(QT_TR_NOOP("Can't export note to ENEX: the selected file "
                                                    "already exists and is not writable"));
                QNWARNING(errorDescription);
                return false;
            }

            QNDEBUG(QStringLiteral("The file selected for ENEX export already exists"));

            int res = questionMessageBox(this, tr("Enex file already exists"), tr("The file selected for ENEX export already exists"),
                                         tr("Do you wish to overwrite the existing file?"));
            if (res != QMessageBox::Ok) {
                QNDEBUG(QStringLiteral("Cancelled overwriting the existing ENEX file"));
                return true;
            }
        }
        else
        {
            QDir enexFileDir = enexFileInfo.absoluteDir();
            if (!enexFileDir.exists())
            {
                bool res = enexFileDir.mkpath(enexFileInfo.absolutePath());
                if (!res) {
                    errorDescription.setBase(QT_TR_NOOP("Can't export note to ENEX: failed "
                                                        "to create folder for the selected file"));
                    QNWARNING(errorDescription);
                    return false;
                }
            }
        }

        QStringList tagNames = (pExportEnexDialog->exportTags() ? m_pUi->tagNameLabelsContainer->tagNames() : QStringList());

        QString enex;
        bool res = m_pUi->noteEditor->exportToEnex(tagNames, enex, errorDescription);
        if (!res) {
            QNDEBUG(QStringLiteral("ENEX export failed: ") << errorDescription);
            return false;
        }

        QFile enexFile(enexFilePath);
        res = enexFile.open(QIODevice::WriteOnly);
        if (!res) {
            errorDescription.setBase(QT_TR_NOOP("Can't export note to ENEX: can't open "
                                                "the target ENEX file for writing"));
            QNWARNING(errorDescription);
            return false;
        }

        QByteArray rawEnexData = enex.toLocal8Bit();
        qint64 bytes = enexFile.write(rawEnexData);
        enexFile.close();

        if (Q_UNLIKELY(bytes != rawEnexData.size())) {
            errorDescription.setBase(QT_TR_NOOP("Writing the ENEX to a file was not completed successfully"));
            errorDescription.details() = QStringLiteral("Bytes written = ") + QString::number(bytes) +
                                         QStringLiteral(" while expected ") + QString::number(rawEnexData.size());
            QNWARNING(errorDescription);
            return false;
        }

        QNDEBUG(QStringLiteral("Successfully exported the note to ENEX"));
        return true;
    }

    QNTRACE(QStringLiteral("Exporting the note to ENEX has been cancelled"));
    return false;
}

void NoteEditorWidget::refreshSpecialIcons()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::refreshSpecialIcons, note local uid = ") << m_noteLocalUid);
    setupSpecialIcons();
}

void NoteEditorWidget::closeEvent(QCloseEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected null pointer to QCloseEvent in NoteEditorWidget's closeEvent"));
        return;
    }

    ErrorString errorDescription;
    NoteSaveStatus::type status = checkAndSaveModifiedNote(errorDescription);
    QNDEBUG(QStringLiteral("Check and save modified note, status: ") << status
            << QStringLiteral(", error description: ") << errorDescription);

    pEvent->accept();
}

bool NoteEditorWidget::eventFilter(QObject * pWatched, QEvent * pEvent)
{
    if (Q_UNLIKELY(!pWatched)) {
        QNWARNING(QStringLiteral("NoteEditorWidget: caught event for null watched object in the eventFilter method"));
        return true;
    }

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("NoteEditorWidget: caught null event in the eventFilter method for object ")
                  << pWatched->objectName() << QStringLiteral(" of class ") << pWatched->metaObject()->className());
        return true;
    }

    if (pWatched == m_pUi->noteNameLineEdit)
    {
        QEvent::Type eventType = pEvent->type();
        if (eventType == QEvent::FocusIn) {
            QNDEBUG(QStringLiteral("Note title editor gained focus"));
        }
        else if (eventType == QEvent::FocusOut) {
            QNDEBUG(QStringLiteral("Note title editor lost focus"));
        }
    }
    else if (pWatched == m_pUi->noteEditor)
    {
        QEvent::Type eventType = pEvent->type();
        if (eventType == QEvent::FocusIn) {
            QNDEBUG(QStringLiteral("Note editor gained focus"));
        }
        else if (eventType == QEvent::FocusOut) {
            QNDEBUG(QStringLiteral("Note editor lost focus"));
        }
    }

    return false;
}

void NoteEditorWidget::onEditorTextBoldToggled()
{
    m_pUi->noteEditor->textBold();
}

void NoteEditorWidget::onEditorTextItalicToggled()
{
    m_pUi->noteEditor->textItalic();
}

void NoteEditorWidget::onEditorTextUnderlineToggled()
{
    m_pUi->noteEditor->textUnderline();
}

void NoteEditorWidget::onEditorTextStrikethroughToggled()
{
    m_pUi->noteEditor->textStrikethrough();
}

void NoteEditorWidget::onEditorTextAlignLeftAction()
{
    if (m_pUi->formatJustifyLeftPushButton->isChecked()) {
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }

    m_pUi->noteEditor->alignLeft();
}

void NoteEditorWidget::onEditorTextAlignCenterAction()
{
    if (m_pUi->formatJustifyCenterPushButton->isChecked()) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }

    m_pUi->noteEditor->alignCenter();
}

void NoteEditorWidget::onEditorTextAlignRightAction()
{
    if (m_pUi->formatJustifyRightPushButton->isChecked()) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }

    m_pUi->noteEditor->alignRight();
}

void NoteEditorWidget::onEditorTextAlignFullAction()
{
    if (m_pUi->formatJustifyFullPushButton->isChecked()) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
    }

    m_pUi->noteEditor->alignFull();
}

void NoteEditorWidget::onEditorTextAddHorizontalLineAction()
{
    m_pUi->noteEditor->insertHorizontalLine();
}

void NoteEditorWidget::onEditorTextIncreaseFontSizeAction()
{
    m_pUi->noteEditor->increaseFontSize();
}

void NoteEditorWidget::onEditorTextDecreaseFontSizeAction()
{
    m_pUi->noteEditor->decreaseFontSize();
}

void NoteEditorWidget::onEditorTextHighlightAction()
{
    m_pUi->noteEditor->textHighlight();
}

void NoteEditorWidget::onEditorTextIncreaseIndentationAction()
{
    m_pUi->noteEditor->increaseIndentation();
}

void NoteEditorWidget::onEditorTextDecreaseIndentationAction()
{
    m_pUi->noteEditor->decreaseIndentation();
}

void NoteEditorWidget::onEditorTextInsertUnorderedListAction()
{
    m_pUi->noteEditor->insertBulletedList();
}

void NoteEditorWidget::onEditorTextInsertOrderedListAction()
{
    m_pUi->noteEditor->insertNumberedList();
}

void NoteEditorWidget::onEditorTextInsertToDoAction()
{
    m_pUi->noteEditor->insertToDoCheckbox();
}

void NoteEditorWidget::onEditorTextFormatAsSourceCodeAction()
{
    m_pUi->noteEditor->formatSelectionAsSourceCode();
}

void NoteEditorWidget::onEditorTextEditHyperlinkAction()
{
    m_pUi->noteEditor->editHyperlinkDialog();
}

void NoteEditorWidget::onEditorTextCopyHyperlinkAction()
{
    m_pUi->noteEditor->copyHyperlink();
}

void NoteEditorWidget::onEditorTextRemoveHyperlinkAction()
{
    m_pUi->noteEditor->removeHyperlink();
}

void NoteEditorWidget::onEditorChooseTextColor(QColor color)
{
    m_pUi->noteEditor->setFontColor(color);
}

void NoteEditorWidget::onEditorChooseBackgroundColor(QColor color)
{
    m_pUi->noteEditor->setBackgroundColor(color);
}

void NoteEditorWidget::onEditorSpellCheckStateChanged(int state)
{
    m_pUi->noteEditor->setSpellcheck(state != Qt::Unchecked);
}

void NoteEditorWidget::onEditorInsertToDoCheckBoxAction()
{
    m_pUi->noteEditor->insertToDoCheckbox();
}

void NoteEditorWidget::onEditorInsertTableDialogAction()
{
    onEditorTextInsertTableDialogRequested();
}

void NoteEditorWidget::onEditorInsertTable(int rows, int columns, double width, bool relativeWidth)
{
    rows = std::max(rows, 1);
    columns = std::max(columns, 1);
    width = std::max(width, 1.0);

    if (relativeWidth) {
        m_pUi->noteEditor->insertRelativeWidthTable(rows, columns, width);
    }
    else {
        m_pUi->noteEditor->insertFixedWidthTable(rows, columns, static_cast<int>(width));
    }

    QNTRACE(QStringLiteral("Inserted table: rows = ") << rows << QStringLiteral(", columns = ") << columns
            << QStringLiteral(", width = ") << width << QStringLiteral(", relative width = ")
            << (relativeWidth ? QStringLiteral("true") : QStringLiteral("false")));
}

void NoteEditorWidget::onUndoAction()
{
    m_pUi->noteEditor->undo();
}

void NoteEditorWidget::onRedoAction()
{
    m_pUi->noteEditor->redo();
}

void NoteEditorWidget::onCopyAction()
{
    m_pUi->noteEditor->copy();
}

void NoteEditorWidget::onCutAction()
{
    m_pUi->noteEditor->cut();
}

void NoteEditorWidget::onPasteAction()
{
    m_pUi->noteEditor->paste();
}

void NoteEditorWidget::onSelectAllAction()
{
    m_pUi->noteEditor->selectAll();
}

void NoteEditorWidget::onSaveNoteAction()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onSaveNoteAction"));

    if (!m_pUi->noteEditor->isModified()) {
        return;
    }

    m_pUi->saveNotePushButton->setEnabled(false);
    m_pUi->noteEditor->convertToNote();
}

void NoteEditorWidget::onSetUseLimitedFonts(bool useLimitedFonts)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onSetUseLimitedFonts: ")
            << (useLimitedFonts ? QStringLiteral("true") : QStringLiteral("false")));

    bool currentlyUsingLimitedFonts = !(m_pUi->limitedFontComboBox->isHidden());

    if (currentlyUsingLimitedFonts == useLimitedFonts) {
        QNDEBUG(QStringLiteral("The setting has not changed, nothing to do"));
        return;
    }

    QString currentFontFamily = (useLimitedFonts
                                 ? m_pUi->limitedFontComboBox->currentText()
                                 : m_pUi->fontComboBox->currentFont().family());

    if (useLimitedFonts)
    {
        QObject::disconnect(m_pUi->fontComboBox, QNSIGNAL(QFontComboBox,currentFontChanged,QFont),
                            this, QNSLOT(NoteEditorWidget,onFontComboBoxFontChanged,QFont));
        m_pUi->fontComboBox->setHidden(true);
        m_pUi->fontComboBox->setDisabled(true);

        setupLimitedFontsComboBox(currentFontFamily);
        m_pUi->limitedFontComboBox->setHidden(false);
        m_pUi->limitedFontComboBox->setDisabled(false);
    }
    else
    {
        QObject::disconnect(m_pUi->limitedFontComboBox, SIGNAL(currentIndexChanged(QString)),
                            this, SLOT(onLimitedFontsComboBoxCurrentIndexChanged(QString)));
        m_pUi->limitedFontComboBox->setHidden(true);
        m_pUi->limitedFontComboBox->setDisabled(true);

        QFont font;
        font.setFamily(currentFontFamily);
        m_pUi->fontComboBox->setFont(font);

        m_pUi->fontComboBox->setHidden(false);
        m_pUi->fontComboBox->setDisabled(false);
        QObject::connect(m_pUi->fontComboBox, QNSIGNAL(QFontComboBox,currentFontChanged,QFont),
                         this, QNSLOT(NoteEditorWidget,onFontComboBoxFontChanged,QFont));
    }
}

void NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem"));

    QWidget * pFocusWidget = qApp->focusWidget();
    if (pFocusWidget)
    {
        NewListItemLineEdit * pNewTagLineEdit = qobject_cast<NewListItemLineEdit*>(pFocusWidget);
        if (pNewTagLineEdit) {
            pNewTagLineEdit->setExpectFocusOut();
        }

        QNTRACE(QStringLiteral("Clearing the focus from the widget having it currently: ") << pFocusWidget);
        pFocusWidget->clearFocus();
    }

    setFocusToEditor();
}

void NoteEditorWidget::onFontComboBoxFontChanged(const QFont & font)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFontComboBoxFontChanged: font family = ")
            << font.family());

    if (m_lastFontComboBoxFontFamily == font.family()) {
        QNTRACE(QStringLiteral("Font family didn't change"));
        return;
    }

    m_lastFontComboBoxFontFamily = font.family();

    if (!m_pCurrentNote) {
        QNTRACE(QStringLiteral("No note is set to the editor, nothing to do"));
        return;
    }

    m_pUi->noteEditor->setFont(font);
}

void NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged(QString fontFamily)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged: ")
            << fontFamily);

    if (fontFamily.trimmed().isEmpty()) {
        QNTRACE(QStringLiteral("Font family is empty, ignoring this update"));
        return;
    }

    removeSurrondingApostrophes(fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE(QStringLiteral("Font family didn't change"));
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    if (m_processingFontFamilyUpdateFromEditor) {
        m_processingFontFamilyUpdateFromEditor = false;
        return;
    }

    if (!m_pCurrentNote) {
        QNTRACE(QStringLiteral("No note is set to the editor, nothing to do"));
        return;
    }

    QFont font;
    font.setFamily(fontFamily);
    m_pUi->noteEditor->setFont(font);
}

void NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged(int index)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged: index = ") << index);

    if (index == m_lastFontSizeComboBoxIndex) {
        QNTRACE(QStringLiteral("Font size hasn't changed"));
        return;
    }

    m_lastFontSizeComboBoxIndex = index;

    if (!m_pCurrentNote) {
        QNTRACE(QStringLiteral("No note is set to the editor, nothing to do"));
        return;
    }

    if (m_lastFontSizeComboBoxIndex < 0) {
        QNTRACE(QStringLiteral("Font size combo box index is negative, won't do anything"));
        return;
    }

    bool conversionResult = false;
    QVariant data = m_pUi->fontSizeComboBox->itemData(index);
    int fontSize = data.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString error(QT_TR_NOOP("Can't process the change of font size combo box index: "
                                     "can't convert combo box item data to int"));
        QNWARNING(error << QStringLiteral(": ") << data);
        Q_EMIT notifyError(error);
        return;
    }

    m_pUi->noteEditor->setFontHeight(fontSize);
}

void NoteEditorWidget::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    if (!m_pCurrentNote || (m_pCurrentNote->localUid() != note.localUid())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onUpdateNoteComplete: note local uid = ") << note.localUid()
            << QStringLiteral(", request id = ") << requestId << QStringLiteral(", update resources = ")
            << (updateResources ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", update tags = ") << (updateTags ? QStringLiteral("true") : QStringLiteral("false")));

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end()) {
        Q_UNUSED(m_updateNoteRequestIds.erase(it))
        Q_EMIT noteSavedInLocalStorage();
        return;
    }

    QNTRACE(QStringLiteral("External update, note: ") << note);

    QList<Resource> backupResources;
    if (!updateResources) {
        backupResources = m_pCurrentNote->resources();
    }

    QStringList backupTagLocalUids;
    bool hadTagLocalUids = m_pCurrentNote->hasTagLocalUids();
    if (!updateTags && hadTagLocalUids) {
        backupTagLocalUids = m_pCurrentNote->tagLocalUids();
    }

    QStringList backupTagGuids;
    bool hadTagGuids = m_pCurrentNote->hasTagGuids();
    if (!updateTags && hadTagGuids) {
        backupTagGuids = m_pCurrentNote->tagGuids();
    }

    QString backupTitle;
    if (m_noteTitleIsEdited) {
        backupTitle = m_pCurrentNote->title();
    }

    *m_pCurrentNote = note;

    if (!updateResources) {
        m_pCurrentNote->setResources(backupResources);
    }

    if (!updateTags) {
        m_pCurrentNote->setTagLocalUids(backupTagLocalUids);
        m_pCurrentNote->setTagGuids(backupTagGuids);
    }

    if (m_noteTitleIsEdited) {
        m_pCurrentNote->setTitle(backupTitle);
    }

    if (note.hasDeletionTimestamp()) {
        QNINFO(QStringLiteral("The note loaded into the editor was deleted: ") << *m_pCurrentNote);
        Q_EMIT invalidated();
        return;
    }

    if (Q_UNLIKELY(m_pCurrentNotebook.isNull()))
    {
        QNDEBUG(QStringLiteral("Current notebook is null - a bit unexpected at this point"));

        if (!m_findCurrentNotebookRequestId.isNull()) {
            QNDEBUG(QStringLiteral("The request to find the current notebook is still active, waiting for it to finish"));
            return;
        }

        const Notebook * pCachedNotebook = Q_NULLPTR;
        if (m_pCurrentNote->hasNotebookLocalUid()) {
            const QString & notebookLocalUid = m_pCurrentNote->notebookLocalUid();
            pCachedNotebook = m_notebookCache.get(notebookLocalUid);
        }

        if (pCachedNotebook)
        {
            m_pCurrentNotebook.reset(new Notebook(*pCachedNotebook));
        }
        else
        {
            if (Q_UNLIKELY(!m_pCurrentNote->hasNotebookLocalUid() && !m_pCurrentNote->hasNotebookGuid())) {
                ErrorString error(QT_TR_NOOP("Note has neither notebook local uid nor notebook guid"));
                if (note.hasTitle()) {
                    error.details() = note.title();
                }
                else {
                    error.details() = m_pCurrentNote->localUid();
                }

                QNWARNING(error << QStringLiteral(", note: ") << *m_pCurrentNote);
                Q_EMIT notifyError(error);
                clear();
                return;
            }

            m_findCurrentNotebookRequestId = QUuid::createUuid();
            Notebook dummy;
            if (m_pCurrentNote->hasNotebookLocalUid()) {
                dummy.setLocalUid(m_pCurrentNote->notebookLocalUid());
            }
            else {
                dummy.setLocalUid(QString());
                dummy.setGuid(m_pCurrentNote->notebookGuid());
            }

            QNTRACE(QStringLiteral("Emitting the request to find the current notebook: ") << dummy
                    << QStringLiteral("\nRequest id = ") << m_findCurrentNotebookRequestId);
            Q_EMIT findNotebook(dummy, m_findCurrentNotebookRequestId);
            return;
        }
    }

    setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);
}

void NoteEditorWidget::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                          ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("NoteEditorWidget::onUpdateNoteFailed: ") << note << QStringLiteral(", update resoures = ")
              << (updateResources ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", update tags = ")
              << (updateTags ? QStringLiteral("true") : QStringLiteral("false"))
              << QStringLiteral(", error description: ") << errorDescription << QStringLiteral("\nRequest id = ") << requestId);

    m_pUi->saveNotePushButton->setEnabled(true);

    ErrorString error(QT_TR_NOOP("Failed to save the updated note"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);
    // NOTE: not clearing out the unsaved stuff because it may be of value to the user

    Q_EMIT noteSaveInLocalStorageFailed();
}

void NoteEditorWidget::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    bool isFindCurrentNoteRequestId = (requestId == m_findCurrentNoteRequestId);
    auto noteLinkInfoIt = (isFindCurrentNoteRequestId
                           ? m_noteLinkInfoByFindNoteRequestIds.end()
                           : m_noteLinkInfoByFindNoteRequestIds.find(requestId));
    if (!isFindCurrentNoteRequestId && (noteLinkInfoIt == m_noteLinkInfoByFindNoteRequestIds.end())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindNoteComplete: request id = ") << requestId
            << QStringLiteral(", with resource binary data = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")));
    QNTRACE(QStringLiteral("Note: ") << note);

    if (isFindCurrentNoteRequestId) {
        m_findCurrentNoteRequestId = QUuid();
        onCurrentNoteFound(note);
        return;
    }

    const NoteLinkInfo & noteLinkInfo = noteLinkInfoIt.value();

    QString titleOrPreview;
    if (note.hasTitle()) {
        titleOrPreview = note.title();
    }
    else if (note.hasContent()) {
        titleOrPreview = note.plainText();
        titleOrPreview.truncate(140);
    }

    QNTRACE(QStringLiteral("Emitting the request to insert the in-app note link: user id = ") << noteLinkInfo.m_userId
            << QStringLiteral(", shard id = ") << noteLinkInfo.m_shardId << QStringLiteral(", note guid = ")
            << noteLinkInfo.m_noteGuid << QStringLiteral(", title or preview = ") << titleOrPreview);
    Q_EMIT insertInAppNoteLink(noteLinkInfo.m_userId, noteLinkInfo.m_shardId, noteLinkInfo.m_noteGuid, titleOrPreview);

    Q_UNUSED(m_noteLinkInfoByFindNoteRequestIds.erase(noteLinkInfoIt))
}

void NoteEditorWidget::onFindNoteFailed(Note note, bool withResourceBinaryData, ErrorString errorDescription,
                                        QUuid requestId)
{
    bool isFindCurrentNoteRequestId = (requestId == m_findCurrentNoteRequestId);
    auto noteLinkInfoIt = (isFindCurrentNoteRequestId
                           ? m_noteLinkInfoByFindNoteRequestIds.end()
                           : m_noteLinkInfoByFindNoteRequestIds.find(requestId));
    if (!isFindCurrentNoteRequestId && (noteLinkInfoIt == m_noteLinkInfoByFindNoteRequestIds.end())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindNoteFailed: request id = ") << requestId << QStringLiteral(", with resource binary data = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", error description: ")
            << errorDescription);
    QNTRACE(QStringLiteral("Note: ") << note);

    if (isFindCurrentNoteRequestId) {
        m_findCurrentNoteRequestId = QUuid();
        clear();
        Q_EMIT notifyError(ErrorString(QT_TR_NOOP("Can't find the note attempted to be selected in the note editor")));
        return;
    }

    const NoteLinkInfo & noteLinkInfo = noteLinkInfoIt.value();

    QNTRACE(QStringLiteral("Emitting the request to insert the in-app note link without the link text as the note was not found: user id = ")
            << noteLinkInfo.m_userId << QStringLiteral(", shard id = ") << noteLinkInfo.m_shardId
            << QStringLiteral(", note guid = ") << noteLinkInfo.m_noteGuid);
    Q_EMIT insertInAppNoteLink(noteLinkInfo.m_userId, noteLinkInfo.m_shardId, noteLinkInfo.m_noteGuid, QString());

    Q_UNUSED(m_noteLinkInfoByFindNoteRequestIds.erase(noteLinkInfoIt))
}

void NoteEditorWidget::onExpungeNoteComplete(Note note, QUuid requestId)
{
    if (!m_pCurrentNote || (m_pCurrentNote->localUid() != note.localUid())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onExpungeNoteComplete: note local uid = ") << note.localUid()
            << QStringLiteral(", request id = ") << requestId);

    m_currentNoteWasExpunged = true;

    // TODO: ideally should allow the choice to either re-save the note or to drop it

    QNINFO(QStringLiteral("The note loaded into the editor was expunged from the local storage: ") << *m_pCurrentNote);
    Q_EMIT invalidated();
}

void NoteEditorWidget::onAddResourceComplete(Resource resource, QUuid requestId)
{
    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        return;
    }

    if (!resource.hasNoteLocalUid() || (resource.noteLocalUid() != m_pCurrentNote->localUid())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onAddResourceComplete: request id = ") << requestId
            << QStringLiteral(", resource: ") << resource);

    if (m_pCurrentNote->hasResources())
    {
        QList<Resource> resources = m_pCurrentNote->resources();
        for(auto it = resources.constBegin(), end = resources.constEnd(); it != end; ++it)
        {
            const Resource & currentResource = *it;
            if (currentResource.localUid() == resource.localUid()) {
                QNDEBUG(QStringLiteral("Found the added resource already belonging to the note"));
                return;
            }
        }
    }

    // Haven't found the added resource within the note's resources, need to add it and update the note editor
    m_pCurrentNote->addResource(resource);
    m_pUi->noteEditor->setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);
}

void NoteEditorWidget::onUpdateResourceComplete(Resource resource, QUuid requestId)
{
    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        return;
    }

    if (!resource.hasNoteLocalUid() || (resource.noteLocalUid() != m_pCurrentNote->localUid())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onUpdateResourceComplete: request id = ") << requestId
            << QStringLiteral(", resource: ") << resource);

    // TODO: ideally should allow the choice to either leave the current version of the note or to reload it

    if (m_pCurrentNote->updateResource(resource)) {
        m_pUi->noteEditor->setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);
    }
}

void NoteEditorWidget::onExpungeResourceComplete(Resource resource, QUuid requestId)
{
    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        return;
    }

    if (!resource.hasNoteLocalUid() || (resource.noteLocalUid() != m_pCurrentNote->localUid())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onExpungeResourceComplete: request id = ") << requestId
            << QStringLiteral(", resource: ") << resource);

    // TODO: ideally should allow the choice to either leave the current version of the note or to reload it

    if (m_pCurrentNote->removeResource(resource)) {
        m_pUi->noteEditor->setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);
    }
}

void NoteEditorWidget::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    if (!m_pCurrentNote || !m_pCurrentNotebook || (m_pCurrentNotebook->localUid() != notebook.localUid())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onUpdateNotebookComplete: notebook = ") << notebook << QStringLiteral("\nRequest id = ")
            << requestId);

    setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);
}

void NoteEditorWidget::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    if (!m_pCurrentNotebook || (m_pCurrentNotebook->localUid() != notebook.localUid())) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onExpungeNotebookComplete: notebook = ") << notebook
            << QStringLiteral("\nRequest id = ") << requestId);

    clear();
}

void NoteEditorWidget::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    if (requestId != m_findCurrentNotebookRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindNotebookComplete: request id = ") << requestId
            << QStringLiteral(", notebook: ") << notebook);

    m_findCurrentNotebookRequestId = QUuid();

    if (Q_UNLIKELY(m_pCurrentNote.isNull())) {
        QNDEBUG(QStringLiteral("Can't process the update of the notebook: no current note is set"));
        clear();
        return;
    }

    m_pCurrentNotebook.reset(new Notebook(notebook));

    setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);

    QNTRACE(QStringLiteral("Emitting resolved signal, note local uid = ") << m_noteLocalUid);
    Q_EMIT resolved();
}

void NoteEditorWidget::onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_findCurrentNotebookRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindNotebookFailed: request id = ") << requestId
            << QStringLiteral(", notebook: ") << notebook << QStringLiteral("\nError description = ") << errorDescription);

    m_findCurrentNotebookRequestId = QUuid();
    clear();
    Q_EMIT notifyError(ErrorString(QT_TR_NOOP("Can't find the note attempted to be selected in the note editor")));
}

void NoteEditorWidget::onNoteTitleEdited(const QString & noteTitle)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onNoteTitleEdited: ") << noteTitle);
    m_noteTitleIsEdited = true;
    m_noteTitleHasBeenEdited = true;
}

void NoteEditorWidget::onNoteEditorModified()
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onNoteEditorModified"));

    m_noteHasBeenModified = true;

    if (!m_pUi->noteEditor->isNoteLoaded()) {
        QNTRACE(QStringLiteral("The note is still being loaded"));
        return;
    }

    if (Q_LIKELY(m_pUi->noteEditor->isModified())) {
        m_pUi->saveNotePushButton->setEnabled(true);
    }
}

void NoteEditorWidget::onNoteTitleUpdated()
{
    QString noteTitle = m_pUi->noteNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(noteTitle);

    QNDEBUG(QStringLiteral("NoteEditorWidget::onNoteTitleUpdated: ") << noteTitle);

    m_noteTitleIsEdited = false;
    m_noteTitleHasBeenEdited = true;

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        // That shouldn't really happen in normal circumstances but it could in theory be some old event
        // which has reached this object after the note has already been cleaned up
        QNDEBUG(QStringLiteral("No current note in the note editor widget! Ignoring the note title update"));
        return;
    }

    if (!m_pCurrentNote->hasTitle() && noteTitle.isEmpty()) {
        QNDEBUG(QStringLiteral("Note's title is still empty, nothing to do"));
        return;
    }

    if (m_pCurrentNote->hasTitle() && (m_pCurrentNote->title() == noteTitle)) {
        QNDEBUG(QStringLiteral("Note's title hasn't changed, nothing to do"));
        return;
    }

    ErrorString error;
    if (!checkNoteTitle(noteTitle, error)) {
        QToolTip::showText(m_pUi->noteNameLineEdit->mapToGlobal(QPoint(0, m_pUi->noteNameLineEdit->height())),
                           error.localizedString());
        return;
    }

    m_pCurrentNote->setTitle(noteTitle);

    // NOTE: indicate early that the note's title has been changed manually
    // and not generated automatically
    qevercloud::NoteAttributes & attributes = m_pCurrentNote->noteAttributes();
    attributes.noteTitleQuality.clear();

    qint64 newModificationTimestamp = QDateTime::currentMSecsSinceEpoch();
    m_pCurrentNote->setModificationTimestamp(newModificationTimestamp);

    m_pCurrentNote->setDirty(true);

    m_isNewNote = false;

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to update note due to note title update: request id = ") << requestId
            << QStringLiteral(", note = ") << *m_pCurrentNote);
    Q_EMIT updateNote(*m_pCurrentNote, /* update resources = */ true, /* update tags = */ false, requestId);

    Q_EMIT titleOrPreviewChanged(titleOrPreview());
}

void NoteEditorWidget::onEditorNoteUpdate(Note note)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onEditorNoteUpdate: note local uid = ") << note.localUid());
    QNTRACE(QStringLiteral("Note: ") << note);

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        // That shouldn't really happen in normal circumstances but it could in theory be some old event
        // which has reached this object after the note has already been cleaned up
        QNDEBUG(QStringLiteral("No current note in the note editor widget! Ignoring the update from the note editor"));
        return;
    }

    QString noteTitle = (m_pCurrentNote->hasTitle() ? m_pCurrentNote->title() : QString());
    *m_pCurrentNote = note;
    m_pCurrentNote->setTitle(noteTitle);

    if (Q_LIKELY(m_pCurrentNotebook)) {
        // Update the note within the tag names container just to avoid any potential race with updates from it
        // messing with other note's parts than tags
        m_pUi->tagNameLabelsContainer->setCurrentNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);
    }

    updateNoteInLocalStorage();
}

void NoteEditorWidget::onEditorNoteUpdateFailed(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onEditorNoteUpdateFailed: ") << error);
    Q_EMIT notifyError(error);

    Q_EMIT conversionToNoteFailed();
}

void NoteEditorWidget::onEditorInAppLinkPasteRequested(QString url, QString userId, QString shardId, QString noteGuid)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onEditorInAppLinkPasteRequested: url = ") << url
            << QStringLiteral(", user id = ") << userId << QStringLiteral(", shard id = ") << shardId
            << QStringLiteral(", note guid = ") << noteGuid);

    NoteLinkInfo noteLinkInfo;
    noteLinkInfo.m_userId = userId;
    noteLinkInfo.m_shardId = shardId;
    noteLinkInfo.m_noteGuid = noteGuid;

    QUuid requestId = QUuid::createUuid();
    m_noteLinkInfoByFindNoteRequestIds[requestId] = noteLinkInfo;
    Note dummyNote;
    dummyNote.unsetLocalUid();
    dummyNote.setGuid(noteGuid);
    QNTRACE(QStringLiteral("Emitting the request to find note by guid for the purpose of in-app note link insertion: request id = ")
            << requestId << QStringLiteral(", note guid = ") << noteGuid);
    Q_EMIT findNote(dummyNote, /* with resource binary data = */ false, requestId);
}

void NoteEditorWidget::onEditorTextBoldStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextBoldStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->fontBoldPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextItalicStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextItalicStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->fontItalicPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextUnderlineStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextUnderlineStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->fontUnderlinePushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextStrikethroughStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextStrikethroughStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->fontStrikethroughPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextAlignLeftStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextAlignLeftStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->formatJustifyLeftPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignCenterStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextAlignCenterStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->formatJustifyCenterPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignRightStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextAlignRightStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->formatJustifyRightPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignFullStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onEditorTextAlignFullStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->formatJustifyFullPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideOrderedListStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextInsideOrderedListStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->formatListOrderedPushButton->setChecked(state);

    if (state) {
        m_pUi->formatListUnorderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->formatListUnorderedPushButton->setChecked(state);

    if (state) {
        m_pUi->formatListOrderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideTableStateChanged(bool state)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextInsideTableStateChanged: ")
            << (state ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    m_pUi->insertTableToolButton->setEnabled(!state);
}

void NoteEditorWidget::onEditorTextFontFamilyChanged(QString fontFamily)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextFontFamilyChanged: ") << fontFamily);

    removeSurrondingApostrophes(fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE(QStringLiteral("Font family didn't change"));
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    QFont currentFont(fontFamily);

    bool useLimitedFonts = !(m_pUi->limitedFontComboBox->isHidden());
    if (useLimitedFonts)
    {
        if (Q_UNLIKELY(!m_pLimitedFontsListModel)) {
            ErrorString errorDescription(QT_TR_NOOP("Can't process the change of font: internal error, no limited fonts "
                                                    "list model is set up"));
            QNWARNING(errorDescription);
            Q_EMIT notifyError(errorDescription);
            return;
        }

        QStringList fontNames = m_pLimitedFontsListModel->stringList();
        int index = fontNames.indexOf(fontFamily);
        if (index < 0)
        {
            QNDEBUG(QStringLiteral("Could not find font name ") << fontFamily
                    << QStringLiteral(" within available font names: ") << fontNames.join(QStringLiteral(", "))
                    << QStringLiteral(", will try to match the font family \"approximately\""));

            for(auto it = fontNames.constBegin(), end = fontNames.constEnd(); it != end; ++it)
            {
                const QString & currentFontFamily = *it;
                if (fontFamily.contains(currentFontFamily, Qt::CaseInsensitive) ||
                    currentFontFamily.contains(fontFamily, Qt::CaseInsensitive))
                {
                    QNDEBUG(QStringLiteral("Note editor's font family ") << fontFamily
                            << QStringLiteral(" appears to correspond to ") << currentFontFamily);
                    index = static_cast<int>(std::distance(fontNames.constBegin(), it));
                    break;
                }
            }
        }

        if (index < 0)
        {
            QNDEBUG(QStringLiteral("Could not find neither exact nor approximate match for the font name ")
                    << fontFamily << QStringLiteral(" within available font names: ") << fontNames.join(QStringLiteral(", "))
                    << QStringLiteral(", will add the missing font name to the list"));

            fontNames << fontFamily;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            fontNames.sort(Qt::CaseInsensitive);
#else
            fontNames.sort();
#endif

            auto it = std::lower_bound(fontNames.constBegin(), fontNames.constEnd(), fontFamily);
            if ((it != fontNames.constEnd()) && (*it == fontFamily)) {
                index = static_cast<int>(std::distance(fontNames.constBegin(), it));
            }
            else {
                index = fontNames.indexOf(fontFamily);
            }

            if (Q_UNLIKELY(index < 0)) {
                QNWARNING(QStringLiteral("Something is weird: can't find the just added font ") << fontFamily
                          << QStringLiteral(" within the list of available fonts where it should have been just added: ")
                          << fontNames.join(QStringLiteral(", ")));
                index = 0;
            }

            m_pLimitedFontsListModel->setStringList(fontNames);
        }

        m_processingFontFamilyUpdateFromEditor = true;
        m_pUi->limitedFontComboBox->setCurrentIndex(index);
        QNTRACE(QStringLiteral("Set limited font combo box index to ") << index
                << QStringLiteral(" corresponding to font family ") << fontFamily);
    }
    else
    {
        m_processingFontFamilyUpdateFromEditor = true;
        m_pUi->fontComboBox->setCurrentFont(currentFont);
        QNTRACE(QStringLiteral("Font family from combo box: ") << m_pUi->fontComboBox->currentFont().family()
                << QStringLiteral(", font family set by QFont's constructor from it: ") << currentFont.family());
    }

    setupFontSizesForFont(currentFont);
}

void NoteEditorWidget::onEditorTextFontSizeChanged(int fontSize)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextFontSizeChanged: ") << fontSize);

    if (m_lastSuggestedFontSize == fontSize) {
        QNTRACE(QStringLiteral("This font size has already been suggested previously"));
        return;
    }

    m_lastSuggestedFontSize = fontSize;

    int fontSizeIndex = m_pUi->fontSizeComboBox->findData(QVariant(fontSize), Qt::UserRole);
    if (fontSizeIndex >= 0)
    {
        m_lastFontSizeComboBoxIndex = fontSizeIndex;
        m_lastActualFontSize = fontSize;

        if (m_pUi->fontSizeComboBox->currentIndex() != fontSizeIndex)
        {
            m_pUi->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE(QStringLiteral("fontSizeComboBox: set current index to ") << fontSizeIndex
                    << QStringLiteral(", found font size = ") << QVariant(fontSize));
        }

        return;
    }

    QNDEBUG(QStringLiteral("Can't find font size ") << fontSize
            << QStringLiteral(" within those listed in font size combobox, will try to choose the closest one instead"));

    const int numFontSizes = m_pUi->fontSizeComboBox->count();
    int currentSmallestDiscrepancy = 1e5;
    int currentClosestIndex = -1;
    int currentClosestFontSize = -1;
    for(int i = 0; i < numFontSizes; ++i)
    {
        QVariant value = m_pUi->fontSizeComboBox->itemData(i, Qt::UserRole);
        bool conversionResult = false;
        int valueInt = value.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING(QStringLiteral("Can't convert value from font size combo box to int: ") << value);
            continue;
        }

        int discrepancy = abs(valueInt - fontSize);
        if (currentSmallestDiscrepancy > discrepancy) {
            currentSmallestDiscrepancy = discrepancy;
            currentClosestIndex = i;
            currentClosestFontSize = valueInt;
            QNTRACE(QStringLiteral("Updated current closest index to ") << i << QStringLiteral(": font size = ") << valueInt);
        }
    }

    if (currentClosestIndex < 0) {
        QNDEBUG(QStringLiteral("Couldn't find closest font size to ") << fontSize);
        return;
    }

    QNTRACE(QStringLiteral("Found closest current font size: ") << currentClosestFontSize << QStringLiteral(", index = ") << currentClosestIndex);

    if ( (m_lastFontSizeComboBoxIndex == currentClosestIndex) &&
         (m_lastActualFontSize == currentClosestFontSize) )
    {
        QNTRACE(QStringLiteral("Neither the font size nor its index within the font combo box have changed"));
        return;
    }

    m_lastFontSizeComboBoxIndex = currentClosestIndex;
    m_lastActualFontSize = currentClosestFontSize;

    if (m_pUi->fontSizeComboBox->currentIndex() != currentClosestIndex) {
        m_pUi->fontSizeComboBox->setCurrentIndex(currentClosestIndex);
    }
}

void NoteEditorWidget::onEditorTextInsertTableDialogRequested()
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorTextInsertTableDialogRequested"));

    QScopedPointer<TableSettingsDialog> tableSettingsDialogHolder(new TableSettingsDialog(this));
    TableSettingsDialog * tableSettingsDialog = tableSettingsDialogHolder.data();
    if (tableSettingsDialog->exec() == QDialog::Rejected) {
        QNTRACE(QStringLiteral("Returned from TableSettingsDialog::exec: rejected"));
        return;
    }

    QNTRACE(QStringLiteral("Returned from TableSettingsDialog::exec: accepted"));
    int numRows = tableSettingsDialog->numRows();
    int numColumns = tableSettingsDialog->numColumns();
    double tableWidth = tableSettingsDialog->tableWidth();
    bool relativeWidth = tableSettingsDialog->relativeWidth();

    if (relativeWidth) {
        m_pUi->noteEditor->insertRelativeWidthTable(numRows, numColumns, tableWidth);
    }
    else {
        m_pUi->noteEditor->insertFixedWidthTable(numRows, numColumns, static_cast<int>(tableWidth));
    }

    m_pUi->noteEditor->setFocus();
}

void NoteEditorWidget::onEditorSpellCheckerNotReady()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onEditorSpellCheckerNotReady"));

    m_pendingEditorSpellChecker = true;
    Q_EMIT notifyError(ErrorString(QT_TR_NOOP("Spell checker is loading dictionaries, please wait")));
}

void NoteEditorWidget::onEditorSpellCheckerReady()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onEditorSpellCheckerReady"));

    if (!m_pendingEditorSpellChecker) {
        return;
    }

    m_pendingEditorSpellChecker = false;
    Q_EMIT notifyError(ErrorString());     // Send the empty message to remove the previous one about the non-ready spell checker
}

void NoteEditorWidget::onEditorHtmlUpdate(QString html)
{
    QNTRACE(QStringLiteral("NoteEditorWidget::onEditorHtmlUpdate"));

    m_lastNoteEditorHtml = html;

    if (Q_LIKELY(m_pUi->noteEditor->isModified())) {
        m_pUi->saveNotePushButton->setEnabled(true);
    }

    if (!m_pUi->noteSourceView->isVisible()) {
        return;
    }

    updateNoteSourceView(html);
}

void NoteEditorWidget::onFindInsideNoteAction()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindInsideNoteAction"));

    if (m_pUi->findAndReplaceWidget->isHidden())
    {
        QString textToFind = m_pUi->noteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_pUi->findAndReplaceWidget->textToFind();
        }
        else {
            m_pUi->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_pUi->findAndReplaceWidget->setHidden(false);
        m_pUi->findAndReplaceWidget->show();
    }

    onFindNextInsideNote(m_pUi->findAndReplaceWidget->textToFind(),
                         m_pUi->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onFindPreviousInsideNoteAction()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindPreviousInsideNoteAction"));

    if (m_pUi->findAndReplaceWidget->isHidden())
    {
        QString textToFind = m_pUi->noteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_pUi->findAndReplaceWidget->textToFind();
        }
        else {
            m_pUi->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_pUi->findAndReplaceWidget->setHidden(false);
        m_pUi->findAndReplaceWidget->show();
    }

    onFindPreviousInsideNote(m_pUi->findAndReplaceWidget->textToFind(),
                             m_pUi->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onReplaceInsideNoteAction()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onReplaceInsideNoteAction"));

    if (m_pUi->findAndReplaceWidget->isHidden() ||
        !m_pUi->findAndReplaceWidget->replaceEnabled())
    {
        QNTRACE(QStringLiteral("At least the replacement part of find and replace widget "
                               "is hidden, will only show it and do nothing else"));

        QString textToFind = m_pUi->noteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_pUi->findAndReplaceWidget->textToFind();
        }
        else {
            m_pUi->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_pUi->findAndReplaceWidget->setHidden(false);
        m_pUi->findAndReplaceWidget->setReplaceEnabled(true);
        m_pUi->findAndReplaceWidget->show();
        return;
    }

    onReplaceInsideNote(m_pUi->findAndReplaceWidget->textToFind(),
                        m_pUi->findAndReplaceWidget->replacementText(),
                        m_pUi->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onFindAndReplaceWidgetClosed()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindAndReplaceWidgetClosed"));
    onFindNextInsideNote(QString(), false);
}

void NoteEditorWidget::onTextToFindInsideNoteEdited(const QString & textToFind)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onTextToFindInsideNoteEdited: ") << textToFind);

    bool matchCase = m_pUi->findAndReplaceWidget->matchCase();
    onFindNextInsideNote(textToFind, matchCase);
}

#define CHECK_FIND_AND_REPLACE_WIDGET_STATE() \
    if (Q_UNLIKELY(m_pUi->findAndReplaceWidget->isHidden())) { \
        QNTRACE(QStringLiteral("Find and replace widget is not shown, nothing to do")); \
        return; \
    }

void NoteEditorWidget::onFindNextInsideNote(const QString & textToFind, const bool matchCase)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindNextInsideNote: text to find = ")
            << textToFind << QStringLiteral(", match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->noteEditor->findNext(textToFind, matchCase);
}

void NoteEditorWidget::onFindPreviousInsideNote(const QString & textToFind, const bool matchCase)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindPreviousInsideNote: text to find = ")
            << textToFind << QStringLiteral(", match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->noteEditor->findPrevious(textToFind, matchCase);
}

void NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged(const bool matchCase)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged: match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()

    QString textToFind = m_pUi->findAndReplaceWidget->textToFind();
    m_pUi->noteEditor->findNext(textToFind, matchCase);
}

void NoteEditorWidget::onReplaceInsideNote(const QString & textToReplace,
                                           const QString & replacementText,
                                           const bool matchCase)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onReplaceInsideNote: text to replace = ")
            << textToReplace << QStringLiteral(", replacement text = ") << replacementText
            << QStringLiteral(", match case = ") << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->findAndReplaceWidget->setReplaceEnabled(true);

    m_pUi->noteEditor->replace(textToReplace, replacementText, matchCase);
}

void NoteEditorWidget::onReplaceAllInsideNote(const QString & textToReplace,
                                              const QString & replacementText,
                                              const bool matchCase)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onReplaceAllInsideNote: text to replace = ")
            << textToReplace << QStringLiteral(", replacement text = ") << replacementText
            << QStringLiteral(", match case = ") << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->findAndReplaceWidget->setReplaceEnabled(true);

    m_pUi->noteEditor->replaceAll(textToReplace, replacementText, matchCase);
}

#undef CHECK_FIND_AND_REPLACE_WIDGET_STATE

void NoteEditorWidget::updateNoteInLocalStorage()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::updateNoteInLocalStorage"));

    if (m_pCurrentNote.isNull()) {
        QNDEBUG(QStringLiteral("No note is set to the editor"));
        return;
    }

    m_pUi->saveNotePushButton->setEnabled(false);

    qint64 newModificationTimestamp = QDateTime::currentMSecsSinceEpoch();
    m_pCurrentNote->setModificationTimestamp(newModificationTimestamp);

    m_pCurrentNote->setDirty(true);

    m_isNewNote = false;

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to update note: request id = ") << requestId
            << QStringLiteral(", note = ") << *m_pCurrentNote);
    Q_EMIT updateNote(*m_pCurrentNote, /* update resources = */ true, /* update tags = */ false, requestId);
}

void NoteEditorWidget::onPrintNoteButtonPressed()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onPrintNoteButtonPressed"));

    ErrorString errorDescription;
    bool res = printNote(errorDescription);
    if (!res) {
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteEditorWidget::onExportNoteToPdfButtonPressed()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onExportNoteToPdfButtonPressed"));

    ErrorString errorDescription;
    bool res = exportNoteToPdf(errorDescription);
    if (!res) {
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteEditorWidget::onExportNoteToEnexButtonPressed()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onExportNoteToEnexButtonPressed"));

    ErrorString errorDescription;
    bool res = exportNoteToEnex(errorDescription);
    if (!res)
    {
        if (errorDescription.isEmpty()) {
            return;
        }

        Q_EMIT notifyError(errorDescription);
    }
}

void NoteEditorWidget::createConnections(LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::createConnections"));

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,updateNote,Note,bool,bool,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,findNote,Note,bool,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,findNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNotebookRequest,Notebook,QUuid));

    // Local signals to note editor's slots
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,insertInAppNoteLink,QString,QString,QString,QString),
                     m_pUi->noteEditor, QNSLOT(NoteEditor,insertInAppNoteLink,QString,QString,QString,QString));

    // localStorageManagerAsync's signals to local slots
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteFailed,Note,bool,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateNoteFailed,Note,bool,bool,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNoteComplete,Note,bool,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,Note,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNoteFailed,Note,bool,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteEditorWidget,onExpungeNoteComplete,Note,QUuid));

    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addResourceComplete,Resource,QUuid),
                     this, QNSLOT(NoteEditorWidget,onAddResourceComplete,Resource,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateResourceComplete,Resource,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateResourceComplete,Resource,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeResourceComplete,Resource,QUuid),
                     this, QNSLOT(NoteEditorWidget,onExpungeResourceComplete,Resource,QUuid));

    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteEditorWidget,onExpungeNotebookComplete,Notebook,QUuid));

    // Connect to font sizes combobox signals
    QObject::connect(m_pUi->fontSizeComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onFontSizesComboBoxCurrentIndexChanged(int)),
                     Qt::UniqueConnection);

    // Connect to note tags widget's signals
    QObject::connect(m_pUi->tagNameLabelsContainer, QNSIGNAL(NoteTagsWidget,newTagLineEditReceivedFocusFromWindowSystem),
                     this, QNSLOT(NoteEditorWidget,onNewTagLineEditReceivedFocusFromWindowSystem));

    // Connect to note title updates
    QObject::connect(m_pUi->noteNameLineEdit, QNSIGNAL(QLineEdit,textEdited,const QString&),
                     this, QNSLOT(NoteEditorWidget,onNoteTitleEdited,const QString&));
    QObject::connect(m_pUi->noteNameLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(NoteEditorWidget,onNoteTitleUpdated));

    // Connect note editor's signals to local signals and slots
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,inAppNoteLinkClicked,QString,QString,QString),
                     this, QNSIGNAL(NoteEditorWidget,inAppNoteLinkClicked,QString,QString,QString));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,inAppNoteLinkPasteRequested,QString,QString,QString,QString),
                     this, QNSLOT(NoteEditorWidget,onEditorInAppLinkPasteRequested,QString,QString,QString,QString));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,convertedToNote,Note),
                     this, QNSLOT(NoteEditorWidget,onEditorNoteUpdate,Note));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,cantConvertToNote,ErrorString),
                     this, QNSLOT(NoteEditorWidget,onEditorNoteUpdateFailed,ErrorString));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,noteEditorHtmlUpdated,QString),
                     this, QNSLOT(NoteEditorWidget,onEditorHtmlUpdate,QString));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,noteLoaded),
                     this, QNSIGNAL(NoteEditorWidget,noteLoaded));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,noteModified),
                     this, QNSLOT(NoteEditorWidget,onNoteEditorModified));

    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textBoldState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextBoldStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textItalicState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextItalicStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textUnderlineState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextUnderlineStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textUnderlineState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextUnderlineStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textStrikethroughState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextStrikethroughStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textAlignLeftState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignLeftStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textAlignCenterState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignCenterStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textAlignRightState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignRightStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textAlignFullState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignFullStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textInsideOrderedListState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsideOrderedListStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textInsideUnorderedListState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsideUnorderedListStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textInsideTableState,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsideTableStateChanged,bool));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textFontFamilyChanged,QString),
                     this, QNSLOT(NoteEditorWidget,onEditorTextFontFamilyChanged,QString));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,textFontSizeChanged,int),
                     this, QNSLOT(NoteEditorWidget,onEditorTextFontSizeChanged,int));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,insertTableDialogRequested),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsertTableDialogRequested));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,spellCheckerNotReady),
                     this, QNSLOT(NoteEditorWidget,onEditorSpellCheckerNotReady));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,spellCheckerReady),
                     this, QNSLOT(NoteEditorWidget,onEditorSpellCheckerReady));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,notifyError,ErrorString),
                     this, QNSIGNAL(NoteEditorWidget,notifyError,ErrorString));

    // Connect find and replace widget actions to local slots
    QObject::connect(m_pUi->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,closed),
                     this, QNSLOT(NoteEditorWidget,onFindAndReplaceWidgetClosed));
    QObject::connect(m_pUi->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,textToFindEdited,const QString&),
                     this, QNSLOT(NoteEditorWidget,onTextToFindInsideNoteEdited,const QString&));
    QObject::connect(m_pUi->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,findNext,const QString&,const bool),
                     this, QNSLOT(NoteEditorWidget,onFindNextInsideNote,const QString&,const bool));
    QObject::connect(m_pUi->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,findPrevious,const QString&,const bool),
                     this, QNSLOT(NoteEditorWidget,onFindPreviousInsideNote,const QString&,const bool));
    QObject::connect(m_pUi->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,searchCaseSensitivityChanged,const bool),
                     this, QNSLOT(NoteEditorWidget,onFindInsideNoteCaseSensitivityChanged,const bool));
    QObject::connect(m_pUi->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,replace,const QString&,const QString&,const bool),
                     this, QNSLOT(NoteEditorWidget,onReplaceInsideNote,const QString&,const QString&,const bool));
    QObject::connect(m_pUi->findAndReplaceWidget, QNSIGNAL(FindAndReplaceWidget,replaceAll,const QString&,const QString&,const bool),
                     this, QNSLOT(NoteEditorWidget,onReplaceAllInsideNote,const QString&,const QString&,const bool));

    // Connect toolbar buttons actions to local slots
    QObject::connect(m_pUi->fontBoldPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextBoldToggled));
    QObject::connect(m_pUi->fontItalicPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextItalicToggled));
    QObject::connect(m_pUi->fontUnderlinePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextUnderlineToggled));
    QObject::connect(m_pUi->fontStrikethroughPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextStrikethroughToggled));
    QObject::connect(m_pUi->formatJustifyLeftPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignLeftAction));
    QObject::connect(m_pUi->formatJustifyCenterPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignCenterAction));
    QObject::connect(m_pUi->formatJustifyRightPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignRightAction));
    QObject::connect(m_pUi->formatJustifyFullPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignFullAction));
    QObject::connect(m_pUi->insertHorizontalLinePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAddHorizontalLineAction));
    QObject::connect(m_pUi->formatIndentMorePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextIncreaseIndentationAction));
    QObject::connect(m_pUi->formatIndentLessPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextDecreaseIndentationAction));
    QObject::connect(m_pUi->formatListUnorderedPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsertUnorderedListAction));
    QObject::connect(m_pUi->formatListOrderedPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsertOrderedListAction));
    QObject::connect(m_pUi->formatAsSourceCodePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorTextFormatAsSourceCodeAction));
    QObject::connect(m_pUi->chooseTextColorToolButton, QNSIGNAL(ColorPickerToolButton,colorSelected,QColor),
                     this, QNSLOT(NoteEditorWidget,onEditorChooseTextColor,QColor));
    QObject::connect(m_pUi->chooseBackgroundColorToolButton, QNSIGNAL(ColorPickerToolButton,colorSelected,QColor),
                     this, QNSLOT(NoteEditorWidget,onEditorChooseBackgroundColor,QColor));
    QObject::connect(m_pUi->spellCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(NoteEditorWidget,onEditorSpellCheckStateChanged,int));
    QObject::connect(m_pUi->insertToDoCheckboxPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onEditorInsertToDoCheckBoxAction));
    QObject::connect(m_pUi->insertTableToolButton, QNSIGNAL(InsertTableToolButton,createdTable,int,int,double,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorInsertTable,int,int,double,bool));
    QObject::connect(m_pUi->printNotePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onPrintNoteButtonPressed));
    QObject::connect(m_pUi->exportNoteToPdfPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onExportNoteToPdfButtonPressed));
    QObject::connect(m_pUi->exportNoteToEnexPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onExportNoteToEnexButtonPressed));

    // Connect toolbar button actions to editor slots
    QObject::connect(m_pUi->copyPushButton, QNSIGNAL(QPushButton,clicked),
                     m_pUi->noteEditor, QNSLOT(NoteEditor,copy));
    QObject::connect(m_pUi->cutPushButton, QNSIGNAL(QPushButton,clicked),
                     m_pUi->noteEditor, QNSLOT(NoteEditor,cut));
    QObject::connect(m_pUi->pastePushButton, QNSIGNAL(QPushButton,clicked),
                     m_pUi->noteEditor, QNSLOT(NoteEditor,paste));
    QObject::connect(m_pUi->undoPushButton, QNSIGNAL(QPushButton,clicked),
                     m_pUi->noteEditor, QNSLOT(NoteEditor,undo));
    QObject::connect(m_pUi->redoPushButton, QNSIGNAL(QPushButton,clicked),
                     m_pUi->noteEditor, QNSLOT(NoteEditor,redo));
    QObject::connect(m_pUi->saveNotePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(NoteEditorWidget,onSaveNoteAction));
}

void NoteEditorWidget::clear()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::clear: note ") << (m_pCurrentNote ? m_pCurrentNote->localUid() : QStringLiteral("<null>")));

    m_pCurrentNote.reset(Q_NULLPTR);
    m_pCurrentNotebook.reset(Q_NULLPTR);
    m_pUi->noteEditor->clear();
    m_pUi->tagNameLabelsContainer->clear();
    m_pUi->noteNameLineEdit->clear();

    m_lastNoteTitleOrPreviewText.clear();

    m_findCurrentNoteRequestId = QUuid();
    m_findCurrentNotebookRequestId = QUuid();
    m_updateNoteRequestIds.clear();
    m_noteLinkInfoByFindNoteRequestIds.clear();

    m_pendingEditorSpellChecker = false;
    m_currentNoteWasExpunged = false;
    m_noteHasBeenModified = false;
}

void NoteEditorWidget::onCurrentNoteFound(const Note & note)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::onCurrentNoteFound"));

    m_pCurrentNote.reset(new Note(note));

    const Notebook * pCachedNotebook = Q_NULLPTR;
    if (m_pCurrentNote->hasNotebookLocalUid()) {
        pCachedNotebook = m_notebookCache.get(m_pCurrentNote->notebookLocalUid());
    }

    if (Q_UNLIKELY(!pCachedNotebook))
    {
        if (!m_findCurrentNotebookRequestId.isNull()) {
            QNDEBUG(QStringLiteral("Couldn't find the notebook in the cache and the request to find it is already active"));
            return;
        }

        m_findCurrentNotebookRequestId = QUuid::createUuid();
        Notebook dummy;
        if (m_pCurrentNote->hasNotebookLocalUid()) {
            dummy.setLocalUid(m_pCurrentNote->notebookLocalUid());
        }
        else {
            dummy.setLocalUid(QString());
            dummy.setGuid(m_pCurrentNote->notebookGuid());
        }

        QNTRACE(QStringLiteral("Emitting the request to find the current notebook: ") << dummy
                << QStringLiteral("\nRequest id = ") << m_findCurrentNotebookRequestId);
        Q_EMIT findNotebook(dummy, m_findCurrentNotebookRequestId);
        return;
    }

    m_pCurrentNotebook.reset(new Notebook(*pCachedNotebook));

    setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);

    QNTRACE(QStringLiteral("Emitting resolved signal, note local uid = ") << m_noteLocalUid);
    Q_EMIT resolved();
}

void NoteEditorWidget::setupSpecialIcons()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setupSpecialIcons"));

    QString enexIconName = QStringLiteral("application-enex");

    if (!QIcon::hasThemeIcon(enexIconName)) {
        QIcon fallbackIcon = QIcon::fromTheme(enexIconName);    // NOTE: it is necessary for the icon to have a name in order to ensure the logic of icon switching works properly
        fallbackIcon.addFile(QStringLiteral(":/icons/oxygen/16x16/mimetypes/application-enex.png"), QSize(16, 16), QIcon::Normal, QIcon::Off);
        fallbackIcon.addFile(QStringLiteral(":/icons/oxygen/22x22/mimetypes/application-enex.png"), QSize(22, 22), QIcon::Normal, QIcon::Off);
        fallbackIcon.addFile(QStringLiteral(":/icons/oxygen/32x32/mimetypes/application-enex.png"), QSize(32, 32), QIcon::Normal, QIcon::Off);
        m_pUi->exportNoteToEnexPushButton->setIcon(fallbackIcon);
    }
    else {
        QIcon themeIcon = QIcon::fromTheme(enexIconName);
        m_pUi->exportNoteToEnexPushButton->setIcon(themeIcon);
    }
}

void NoteEditorWidget::setupFontsComboBox()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setupFontsComboBox"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);

    bool useLimitedFonts = false;
    if (appSettings.contains(USE_LIMITED_SET_OF_FONTS)) {
        useLimitedFonts = appSettings.value(USE_LIMITED_SET_OF_FONTS).toBool();
        QNDEBUG(QStringLiteral("Use limited fonts setting from settings: ")
                << (useLimitedFonts ? QStringLiteral("true") : QStringLiteral("false")));
    }
    else {
        useLimitedFonts = (m_currentAccount.type() == Account::Type::Evernote);
    }

    appSettings.endGroup();

    if (useLimitedFonts) {
        m_pUi->fontComboBox->setHidden(true);
        m_pUi->fontComboBox->setDisabled(true);
        setupLimitedFontsComboBox();
    }
    else {
        m_pUi->limitedFontComboBox->setHidden(true);
        m_pUi->limitedFontComboBox->setDisabled(true);
        QObject::connect(m_pUi->fontComboBox, QNSIGNAL(QFontComboBox,currentFontChanged,QFont),
                         this, QNSLOT(NoteEditorWidget,onFontComboBoxFontChanged,QFont));
    }
}

void NoteEditorWidget::setupLimitedFontsComboBox(const QString & startupFont)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setupLimitedFontsComboBox: startup font = ")
            << startupFont);

    QStringList limitedFontNames;
    limitedFontNames.reserve(8);
    limitedFontNames << QStringLiteral("Courier New");
    limitedFontNames << QStringLiteral("Georgia");
    limitedFontNames << QStringLiteral("Gotham");
    limitedFontNames << QStringLiteral("Helvetica");
    limitedFontNames << QStringLiteral("Trebuchet");
    limitedFontNames << QStringLiteral("Times");
    limitedFontNames << QStringLiteral("Times New Roman");
    limitedFontNames << QStringLiteral("Verdana");

    delete m_pLimitedFontsListModel;
    m_pLimitedFontsListModel = new QStringListModel(this);
    m_pLimitedFontsListModel->setStringList(limitedFontNames);
    m_pUi->limitedFontComboBox->setModel(m_pLimitedFontsListModel);

    int currentIndex = limitedFontNames.indexOf(startupFont);
    if (currentIndex < 0) {
        currentIndex = 0;
    }

    m_pUi->limitedFontComboBox->setCurrentIndex(currentIndex);

    LimitedFontsDelegate * pDelegate = qobject_cast<LimitedFontsDelegate*>(m_pUi->limitedFontComboBox->itemDelegate());
    if (!pDelegate) {
        pDelegate = new LimitedFontsDelegate(m_pUi->limitedFontComboBox);
        m_pUi->limitedFontComboBox->setItemDelegate(pDelegate);
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    // NOTE: without this the combobox looks pretty ugly in Qt4
    QLineEdit * pLineEdit = m_pUi->limitedFontComboBox->lineEdit();
    if (pLineEdit) {
        pLineEdit->setStyleSheet(QStringLiteral("padding-left: 2px;"));
    }
#endif

    QObject::connect(m_pUi->limitedFontComboBox, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onLimitedFontsComboBoxCurrentIndexChanged(QString)));
}

void NoteEditorWidget::setupFontSizesComboBox()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setupFontSizesComboBox"));

    bool useLimitedFonts = !(m_pUi->limitedFontComboBox->isHidden());

    QFont currentFont;
    if (useLimitedFonts) {
        currentFont.setFamily(m_pUi->limitedFontComboBox->currentText());
    }
    else {
        currentFont = m_pUi->fontComboBox->currentFont();
    }

    setupFontSizesForFont(currentFont);
}

void NoteEditorWidget::setupFontSizesForFont(const QFont & font)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setupFontSizesForFont: family = ") << font.family()
            << QStringLiteral(", style name = ") << font.styleName());

    QFontDatabase fontDatabase;
    QList<int> fontSizes = fontDatabase.pointSizes(font.family(), font.styleName());
    if (fontSizes.isEmpty()) {
        QNTRACE(QStringLiteral("Couldn't find point sizes for font family ") << font.family()
                << QStringLiteral(", will use standard sizes instead"));
        fontSizes = fontDatabase.standardSizes();
    }

    int currentFontSize = -1;
    int currentFontSizeIndex = m_pUi->fontSizeComboBox->currentIndex();

    QList<int> currentFontSizes;
    int currentCount = m_pUi->fontSizeComboBox->count();
    currentFontSizes.reserve(currentCount);
    for(int i = 0; i < currentCount; ++i)
    {
        bool conversionResult = false;
        QVariant data = m_pUi->fontSizeComboBox->itemData(i);
        int fontSize = data.toInt(&conversionResult);
        if (conversionResult)
        {
            currentFontSizes << fontSize;
            if (i == currentFontSizeIndex) {
                currentFontSize = fontSize;
            }
        }
    }

    if (currentFontSizes == fontSizes) {
        QNDEBUG(QStringLiteral("No need to update the items within font sizes combo box: none of them have changed"));
        return;
    }

    QObject::disconnect(m_pUi->fontSizeComboBox, SIGNAL(currentIndexChanged(int)),
                        this, SLOT(onFontSizesComboBoxCurrentIndexChanged(int)));

    m_lastFontSizeComboBoxIndex = 0;
    m_pUi->fontSizeComboBox->clear();
    int numFontSizes = fontSizes.size();
    QNTRACE(QStringLiteral("Found ") << numFontSizes << QStringLiteral(" font sizes for font family ")
            << font.family());

    for(int i = 0; i < numFontSizes; ++i) {
        m_pUi->fontSizeComboBox->addItem(QString::number(fontSizes[i]), QVariant(fontSizes[i]));
        QNTRACE(QStringLiteral("Added item ") << fontSizes[i] << QStringLiteral("pt for index ") << i);
    }

    m_lastFontSizeComboBoxIndex = -1;

    bool setFontSizeIndex = false;
    if (currentFontSize > 0)
    {
        int fontSizeIndex = fontSizes.indexOf(currentFontSize);
        if (fontSizeIndex >= 0) {
            m_pUi->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE(QStringLiteral("Setting the current font size to its previous value: ")
                    << currentFontSize);
            setFontSizeIndex = true;
        }
    }

    if (!setFontSizeIndex && (currentFontSize != 12))
    {
        // Try to look for font size 12 as the sanest default font size
        int fontSizeIndex = fontSizes.indexOf(12);
        if (fontSizeIndex >= 0) {
            m_pUi->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE(QStringLiteral("Setting the current font size to the default value of 12"));
            setFontSizeIndex = true;
        }
    }

    if (!setFontSizeIndex)
    {
        // Try to find any font size between 10 and 20, should be good enough
        for(int i = 0; i < numFontSizes; ++i)
        {
            int fontSize = fontSizes[i];
            if ((fontSize >= 10) && (fontSize <= 20)) {
                m_pUi->fontSizeComboBox->setCurrentIndex(i);
                QNTRACE(QStringLiteral("Setting the current font size to the default value of ")
                        << fontSize);
                setFontSizeIndex = true;
                break;
            }
        }
    }

    if (!setFontSizeIndex && !fontSizes.isEmpty())
    {
        // All attempts to pick some sane font size have failed,
        // will just take the median (or only) font size
        int index = numFontSizes / 2;
        m_pUi->fontSizeComboBox->setCurrentIndex(index);
        QNTRACE(QStringLiteral("Setting the current font size to the median value of ")
                << fontSizes.at(index));
        setFontSizeIndex = true;
    }

    QObject::connect(m_pUi->fontSizeComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onFontSizesComboBoxCurrentIndexChanged(int)),
                     Qt::UniqueConnection);
}

void NoteEditorWidget::updateNoteSourceView(const QString & html)
{
    m_pUi->noteSourceView->setPlainText(html);
}

void NoteEditorWidget::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setCurrentNoteAndNotebook"));
    QNTRACE(QStringLiteral("Note: ") << note << QStringLiteral("\nNotebook: ") << notebook);

    m_pUi->noteNameLineEdit->show();
    m_pUi->tagNameLabelsContainer->show();

    if (!m_noteTitleIsEdited && note.hasTitle())
    {
        QString title = note.title();
        m_pUi->noteNameLineEdit->setText(title);
        if (m_lastNoteTitleOrPreviewText != title) {
            m_lastNoteTitleOrPreviewText = title;
            Q_EMIT titleOrPreviewChanged(m_lastNoteTitleOrPreviewText);
        }
    }
    else if (!m_noteTitleIsEdited)
    {
        m_pUi->noteNameLineEdit->clear();

        QString previewText;
        if (note.hasContent()) {
            previewText = note.plainText();
            previewText.truncate(140);
        }

        if (previewText != m_lastNoteTitleOrPreviewText) {
            m_lastNoteTitleOrPreviewText = previewText;
            Q_EMIT titleOrPreviewChanged(m_lastNoteTitleOrPreviewText);
        }
    }

    m_pUi->noteEditor->setNoteAndNotebook(note, notebook);
    m_pUi->tagNameLabelsContainer->setCurrentNoteAndNotebook(note, notebook);

    m_pUi->printNotePushButton->setDisabled(false);
    m_pUi->exportNoteToPdfPushButton->setDisabled(false);
    m_pUi->exportNoteToEnexPushButton->setDisabled(false);
}

QString NoteEditorWidget::blankPageHtml() const
{
    QString html = QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                                  "<html><head>"
                                  "<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\" />"
                                  "<style>"
                                  "body {"
                                  "background-color: ");

    QColor backgroundColor = palette().color(QPalette::Window).darker(115);
    html += backgroundColor.name();

    html += QStringLiteral(";"
                           "color: ");
    QColor foregroundColor = palette().color(QPalette::WindowText);
    html += foregroundColor.name();

    html += QStringLiteral(";"
                           "-webkit-user-select: none;"
                           "}"
                           ".outer {"
                           "    display: table;"
                           "    position: absolute;"
                           "    height: 95%;"
                           "    width: 95%;"
                           "}"
                           ".middle {"
                           "    display: table-cell;"
                           "    vertical-align: middle;"
                           "}"
                           ".inner {"
                           "    text-align: center;"
                           "}"
                           "</style><title></title></head>"
                           "<body><div class=\"outer\"><div class=\"middle\"><div class=\"inner\">\n\n\n");

    html += tr("Please select some existing note or create a new one");

    html += QStringLiteral("</div></div></div></body></html>");

    return html;
}

void NoteEditorWidget::setupBlankEditor()
{
    QNDEBUG(QStringLiteral("NoteEditorWidget::setupBlankEditor"));

    m_pUi->printNotePushButton->setDisabled(true);
    m_pUi->exportNoteToPdfPushButton->setDisabled(true);
    m_pUi->exportNoteToEnexPushButton->setDisabled(true);

    m_pUi->noteNameLineEdit->hide();
    m_pUi->tagNameLabelsContainer->hide();

    QString initialHtml = blankPageHtml();
    m_pUi->noteEditor->setBlankPageHtml(initialHtml);

    m_pUi->findAndReplaceWidget->setHidden(true);
    m_pUi->noteSourceView->setHidden(true);
}

bool NoteEditorWidget::checkNoteTitle(const QString & title, ErrorString & errorDescription)
{
    if (title.isEmpty()) {
        return true;
    }

    return Note::validateTitle(title, &errorDescription);
}

void NoteEditorWidget::removeSurrondingApostrophes(QString & str) const
{
    if (str.startsWith(QStringLiteral("\'")) && str.endsWith(QStringLiteral("\'")) && (str.size() > 1)) {
        str = str.mid(1, str.size() - 2);
        QNTRACE(QStringLiteral("Removed apostrophes: ") << str);
    }
}

QTextStream & NoteEditorWidget::NoteLinkInfo::print(QTextStream & strm) const
{
    strm << QStringLiteral("User id = ") << m_userId << QStringLiteral(", shard id = ") << m_shardId
         << QStringLiteral(", note guid = ") << m_noteGuid;
    return strm;
}

} // namespace quentier

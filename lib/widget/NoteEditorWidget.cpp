/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include "FindAndReplaceWidget.h"
#include "NewListItemLineEdit.h"
#include "NoteTagsWidget.h"

#include "color-picker-tool-button/ColorPickerToolButton.h"
#include "insert-table-tool-button/InsertTableToolButton.h"
#include "insert-table-tool-button/TableSettingsDialog.h"

#include <lib/delegate/LimitedFontsDelegate.h>
#include <lib/enex/EnexExportDialog.h>
#include <lib/model/tag/TagModel.h>
#include <lib/preferences/defaults/NoteEditor.h>
#include <lib/preferences/keys/Enex.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/NoteEditor.h>
#include <lib/utility/BasicXMLSyntaxHighlighter.h>

// Doh, Qt Designer's inability to work with namespaces in the expected way
// is deeply disappointing
#include <quentier/note_editor/INoteEditorBackend.h>
#include <quentier/note_editor/NoteEditor.h>

using quentier::ColorPickerToolButton;
using quentier::FindAndReplaceWidget;
using quentier::InsertTableToolButton;
using quentier::NoteEditor;
using quentier::NoteTagsWidget;

#include "ui_NoteEditorWidget.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/note_editor/SpellChecker.h>
#include <quentier/types/NoteUtils.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StandardPaths.h>

#include <qevercloud/types/Resource.h>

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMimeData>
#include <QPalette>
#include <QPrintDialog>
#include <QStringListModel>
#include <QThread>
#include <QTimer>
#include <QToolTip>

#include <memory>

namespace quentier {

NoteEditorWidget::NoteEditorWidget(
    Account account,
    LocalStorageManagerAsync & localStorageManagerAsync,
    SpellChecker & spellChecker, QThread * pBackgroundJobsThread,
    NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
    TagModel & tagModel, QUndoStack * pUndoStack, QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::NoteEditorWidget), m_noteCache(noteCache),
    m_notebookCache(notebookCache), m_tagCache(tagCache),
    m_currentAccount(std::move(account)), m_pUndoStack(pUndoStack)
{
    m_pUi->setupUi(this);
    setAcceptDrops(true);

    setupSpecialIcons();
    setupFontsComboBox();
    setupFontSizesComboBox();

    // Originally the NoteEditorWidget is not a window, so hide these two
    // buttons, they are for the separate-window mode only
    m_pUi->printNotePushButton->setHidden(true);
    m_pUi->exportNoteToPdfPushButton->setHidden(true);

    m_pUi->noteEditor->initialize(
        localStorageManagerAsync, spellChecker, m_currentAccount,
        pBackgroundJobsThread);

    m_pUi->saveNotePushButton->setEnabled(false);
    m_pUi->noteNameLineEdit->installEventFilter(this);
    m_pUi->noteEditor->setUndoStack(m_pUndoStack.data());

    setupNoteEditorColors();
    setupBlankEditor();

    m_pUi->noteEditor->backend()->widget()->installEventFilter(this);

    auto * highlighter =
        new BasicXMLSyntaxHighlighter(m_pUi->noteSourceView->document());

    Q_UNUSED(highlighter);

    m_pUi->tagNameLabelsContainer->setTagModel(&tagModel);

    m_pUi->tagNameLabelsContainer->setLocalStorageManagerThreadWorker(
        localStorageManagerAsync);

    createConnections(localStorageManagerAsync);
    QWidget::setAttribute(Qt::WA_DeleteOnClose, /* on = */ true);
}

NoteEditorWidget::~NoteEditorWidget() = default;

QString NoteEditorWidget::noteLocalId() const
{
    return m_noteLocalId;
}

bool NoteEditorWidget::isNewNote() const noexcept
{
    return m_isNewNote;
}

void NoteEditorWidget::setNoteLocalId(
    const QString & noteLocalId, const bool isNewNote)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::setNoteLocalId: "
            << noteLocalId
            << ", is new note = " << (isNewNote ? "true" : "false"));

    if (m_noteLocalId == noteLocalId) {
        QNDEBUG(
            "widget:note_editor",
            "Note local id is already set to "
                << "the editor, nothing to do");
        return;
    }

    clear();
    m_noteLocalId = noteLocalId;

    if (noteLocalId.isEmpty()) {
        setupBlankEditor();
        return;
    }

    m_isNewNote = isNewNote;

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::noteAndNotebookFoundInLocalStorage,
        this, &NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::noteNotFound, this,
        &NoteEditorWidget::onNoteNotFoundInLocalStorage,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    m_pUi->noteEditor->setCurrentNoteLocalId(noteLocalId);
}

bool NoteEditorWidget::isResolved() const noexcept
{
    return m_pCurrentNote && m_pCurrentNotebook;
}

bool NoteEditorWidget::isModified() const
{
    return m_pCurrentNote &&
        (m_pUi->noteEditor->isModified() || m_noteTitleIsEdited);
}

bool NoteEditorWidget::hasBeenModified() const
{
    return m_noteHasBeenModified || m_noteTitleHasBeenEdited;
}

qint64 NoteEditorWidget::idleTime() const
{
    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        return -1;
    }

    return m_pUi->noteEditor->idleTime();
}

QString NoteEditorWidget::titleOrPreview() const
{
    if (Q_UNLIKELY(!m_pCurrentNote)) {
        return {};
    }

    if (m_pCurrentNote->title()) {
        return *m_pCurrentNote->title();
    }

    if (m_pCurrentNote->content()) {
        QString previewText =
            noteContentToPlainText(*m_pCurrentNote->content());

        previewText.truncate(140);
        return previewText;
    }

    return {};
}

const qevercloud::Note * NoteEditorWidget::currentNote() const noexcept
{
    return m_pCurrentNote.get();
}

bool NoteEditorWidget::isNoteSourceShown() const noexcept
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

NoteEditorWidget::NoteSaveStatus
NoteEditorWidget::checkAndSaveModifiedNote(ErrorString & errorDescription)
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::checkAndSaveModifiedNote");

    if (!m_pCurrentNote) {
        QNDEBUG("widget:note_editor", "No note is set to the editor");
        return NoteSaveStatus::Ok;
    }

    if (m_pCurrentNote->deleted()) {
        QNDEBUG(
            "widget:note_editor",
            "The note is deleted which means it just got deleted and "
                << "the editor is closing => there is no need to "
                << "save whatever is left in the editor for this note");
        return NoteSaveStatus::Ok;
    }

    const bool noteContentModified = m_pUi->noteEditor->isModified();

    if (!m_noteTitleIsEdited && !noteContentModified) {
        QNDEBUG("widget:note_editor", "Note is not modified, nothing to save");
        return NoteSaveStatus::Ok;
    }

    bool noteTitleUpdated = false;

    if (m_noteTitleIsEdited) {
        QString noteTitle = m_pUi->noteNameLineEdit->text().trimmed();
        m_stringUtils.removeNewlines(noteTitle);

        if (noteTitle.isEmpty() && m_pCurrentNote->title()) {
            m_pCurrentNote->setTitle(QString{});
            noteTitleUpdated = true;
        }
        else if (
            !noteTitle.isEmpty() &&
            (!m_pCurrentNote->title() ||
             (*m_pCurrentNote->title() != noteTitle)))
        {
            ErrorString error;
            if (checkNoteTitle(noteTitle, error)) {
                m_pCurrentNote->setTitle(noteTitle);
                noteTitleUpdated = true;
            }
            else {
                QNDEBUG(
                    "widget:note_editor",
                    "Couldn't save the note title: " << error);
            }
        }
    }

    if (noteTitleUpdated && m_pCurrentNote->attributes()) {
        // NOTE: indicating early that the note's title has been changed
        // manually and not generated automatically
        m_pCurrentNote->mutableAttributes()->mutableNoteTitleQuality().reset();
    }

    if (noteContentModified || noteTitleUpdated) {
        ApplicationSettings appSettings;
        appSettings.beginGroup(preferences::keys::noteEditorGroup);

        const auto editorConvertToNoteTimeoutData = appSettings.value(
            preferences::keys::noteEditorConvertToNoteTimeout);

        appSettings.endGroup();

        bool conversionResult = false;

        int editorConvertToNoteTimeout =
            editorConvertToNoteTimeoutData.toInt(&conversionResult);

        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(
                "widget:note_editor",
                "Can't read the timeout for note editor to note conversion "
                    << "from the application settings, "
                    << "fallback to the default value of "
                    << preferences::defaults::convertToNoteTimeout
                    << " milliseconds");

            editorConvertToNoteTimeout =
                preferences::defaults::convertToNoteTimeout;
        }
        else {
            editorConvertToNoteTimeout =
                std::max(editorConvertToNoteTimeout, 100);
        }

        if (m_pConvertToNoteDeadlineTimer) {
            m_pConvertToNoteDeadlineTimer->deleteLater();
        }

        m_pConvertToNoteDeadlineTimer = new QTimer(this);
        m_pConvertToNoteDeadlineTimer->setSingleShot(true);

        EventLoopWithExitStatus eventLoop;

        QObject::connect(
            m_pConvertToNoteDeadlineTimer, &QTimer::timeout, &eventLoop,
            &EventLoopWithExitStatus::exitAsTimeout);

        QObject::connect(
            this, &NoteEditorWidget::noteSavedInLocalStorage, &eventLoop,
            &EventLoopWithExitStatus::exitAsSuccess);

        QObject::connect(
            this, &NoteEditorWidget::noteSaveInLocalStorageFailed, &eventLoop,
            &EventLoopWithExitStatus::exitAsFailure);

        QObject::connect(
            this, &NoteEditorWidget::conversionToNoteFailed, &eventLoop,
            &EventLoopWithExitStatus::exitAsFailure);

        m_pConvertToNoteDeadlineTimer->start(editorConvertToNoteTimeout);

        QTimer::singleShot(0, this, SLOT(updateNoteInLocalStorage()));

        Q_UNUSED(eventLoop.exec(QEventLoop::ExcludeUserInputEvents))
        const auto status = eventLoop.exitStatus();

        if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
            errorDescription.setBase(
                QT_TR_NOOP("Failed to convert the editor contents to note"));
            QNWARNING("widget:note_editor", errorDescription);
            return NoteSaveStatus::Failed;
        }

        if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
            errorDescription.setBase(
                QT_TR_NOOP("The conversion of note editor contents to note "
                           "failed to finish in time"));
            QNWARNING("widget:note_editor", errorDescription);
            return NoteSaveStatus::Timeout;
        }
    }

    return NoteSaveStatus::Ok;
}

bool NoteEditorWidget::isSeparateWindow() const
{
    const Qt::WindowFlags flags = windowFlags();
    return flags.testFlag(Qt::Window);
}

bool NoteEditorWidget::makeSeparateWindow()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::makeSeparateWindow");

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
    QNDEBUG("widget:note_editor", "NoteEditorWidget::setFocusToEditor");
    m_pUi->noteEditor->setFocus();
}

bool NoteEditorWidget::printNote(ErrorString & errorDescription)
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::printNote");

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't print note: no note is set to the editor"));
        QNDEBUG("widget:note_editor", errorDescription);
        return false;
    }

    QPrinter printer;
    auto pPrintDialog = std::make_unique<QPrintDialog>(&printer, this);
    pPrintDialog->setWindowModality(Qt::WindowModal);

    QAbstractPrintDialog::PrintDialogOptions options;
    options |= QAbstractPrintDialog::PrintToFile;
    options |= QAbstractPrintDialog::PrintCollateCopies;

    pPrintDialog->setOptions(options);

    if (pPrintDialog->exec() == QDialog::Accepted) {
        return m_pUi->noteEditor->print(printer, errorDescription);
    }

    QNTRACE("widget:note_editor", "Note printing has been cancelled");
    return false;
}

bool NoteEditorWidget::exportNoteToPdf(ErrorString & errorDescription)
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::exportNoteToPdf");

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't export note to pdf: no note within the note "
                       "editor widget"));
        return false;
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QString lastExportNoteToPdfPath =
        appSettings.value(preferences::keys::lastExportNoteToPdfPath)
            .toString();

    appSettings.endGroup();

    if (lastExportNoteToPdfPath.isEmpty()) {
        lastExportNoteToPdfPath = documentsPath();
    }

    auto pFileDialog = std::make_unique<QFileDialog>(
        this, tr("Please select the output pdf file"), lastExportNoteToPdfPath);

    pFileDialog->setWindowModality(Qt::WindowModal);
    pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    pFileDialog->setFileMode(QFileDialog::AnyFile);
    pFileDialog->setDefaultSuffix(QStringLiteral("pdf"));

    QString suggestedFileName;
    if (m_pCurrentNote->title()) {
        suggestedFileName = *m_pCurrentNote->title();
    }
    else if (m_pCurrentNote->content()) {
        suggestedFileName =
            noteContentToPlainText(*m_pCurrentNote->content()).simplified();

        suggestedFileName.truncate(30);
    }

    if (!suggestedFileName.isEmpty()) {
        suggestedFileName += QStringLiteral(".pdf");
        pFileDialog->selectFile(suggestedFileName);
    }

    if (pFileDialog->exec() == QDialog::Accepted) {
        const QStringList selectedFiles = pFileDialog->selectedFiles();
        const int numSelectedFiles = selectedFiles.size();

        if (numSelectedFiles == 0) {
            errorDescription.setBase(
                QT_TR_NOOP("No pdf file was selected to export the note into"));
            return false;
        }

        if (numSelectedFiles > 1) {
            errorDescription.setBase(
                QT_TR_NOOP("More than one file were selected as output pdf "
                           "files"));
            errorDescription.details() =
                selectedFiles.join(QStringLiteral(", "));
            return false;
        }

        const QFileInfo selectedFileInfo{selectedFiles[0]};
        if (selectedFileInfo.exists()) {
            if (Q_UNLIKELY(!selectedFileInfo.isFile())) {
                errorDescription.setBase(
                    QT_TR_NOOP("No file to save the pdf into was selected"));
                return false;
            }

            if (Q_UNLIKELY(!selectedFileInfo.isWritable())) {
                errorDescription.setBase(
                    QT_TR_NOOP("The selected file already exists and it is not "
                               "writable"));
                errorDescription.details() = selectedFiles[0];
                return false;
            }

            const int confirmOverwrite = questionMessageBox(
                this, tr("Overwrite existing file"),
                tr("Confirm the choice to overwrite the existing file"),
                tr("The selected pdf file already exists. Are you sure you "
                   "want to overwrite this file?"));

            if (confirmOverwrite != QMessageBox::Ok) {
                QNDEBUG("widget:note_editor", "Cancelled the export to pdf");
                return false;
            }
        }

        lastExportNoteToPdfPath = selectedFileInfo.absolutePath();
        if (!lastExportNoteToPdfPath.isEmpty()) {
            appSettings.beginGroup(preferences::keys::noteEditorGroup);

            appSettings.setValue(
                preferences::keys::lastExportNoteToPdfPath,
                lastExportNoteToPdfPath);

            appSettings.endGroup();
        }

        return m_pUi->noteEditor->exportToPdf(
            selectedFiles[0], errorDescription);
    }

    QNTRACE(
        "widget:note_editor",
        "Exporting the note to pdf has been cancelled");

    return false;
}

bool NoteEditorWidget::exportNoteToEnex(ErrorString & errorDescription)
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::exportNoteToEnex");

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QString lastExportNoteToEnexPath =
        appSettings.value(preferences::keys::lastExportNotesToEnexPath)
            .toString();

    appSettings.endGroup();

    if (lastExportNoteToEnexPath.isEmpty()) {
        lastExportNoteToEnexPath = documentsPath();
    }

    auto pExportEnexDialog = std::make_unique<EnexExportDialog>(
        m_currentAccount, this, titleOrPreview());

    pExportEnexDialog->setWindowModality(Qt::WindowModal);

    if (pExportEnexDialog->exec() == QDialog::Accepted) {
        QNDEBUG("widget:note_editor", "Confirmed ENEX export");

        const QString enexFilePath = pExportEnexDialog->exportEnexFilePath();

        const QFileInfo enexFileInfo{enexFilePath};
        if (enexFileInfo.exists()) {
            if (!enexFileInfo.isWritable()) {
                errorDescription.setBase(
                    QT_TR_NOOP("Can't export note to ENEX: the selected file "
                               "already exists and is not writable"));
                QNWARNING("widget:note_editor", errorDescription);
                return false;
            }

            QNDEBUG(
                "widget:note_editor",
                "The file selected for ENEX export already exists");

            const int res = questionMessageBox(
                this, tr("Enex file already exists"),
                tr("The file selected for ENEX export already exists"),
                tr("Do you wish to overwrite the existing file?"));

            if (res != QMessageBox::Ok) {
                QNDEBUG(
                    "widget:note_editor",
                    "Cancelled overwriting "
                        << "the existing ENEX file");
                return true;
            }
        }
        else {
            auto enexFileDir = enexFileInfo.absoluteDir();
            if (!enexFileDir.exists()) {
                if (!enexFileDir.mkpath(enexFileInfo.absolutePath())) {
                    errorDescription.setBase(
                        QT_TR_NOOP("Can't export note to ENEX: failed to "
                                   "create folder for the selected file"));
                    QNWARNING("widget:note_editor", errorDescription);
                    return false;
                }
            }
        }

        const QStringList tagNames =
            (pExportEnexDialog->exportTags()
                 ? m_pUi->tagNameLabelsContainer->tagNames()
                 : QStringList());

        QString enex;
        if (!m_pUi->noteEditor->exportToEnex(tagNames, enex, errorDescription))
        {
            QNDEBUG(
                "widget:note_editor",
                "ENEX export failed: " << errorDescription);
            return false;
        }

        QFile enexFile{enexFilePath};
        if (!enexFile.open(QIODevice::WriteOnly)) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't export note to ENEX: can't open the target "
                           "ENEX file for writing"));
            QNWARNING("widget:note_editor", errorDescription);
            return false;
        }

        const QByteArray rawEnexData = enex.toLocal8Bit();
        const qint64 bytes = enexFile.write(rawEnexData);
        enexFile.close();

        if (Q_UNLIKELY(bytes != rawEnexData.size())) {
            errorDescription.setBase(
                QT_TR_NOOP("Writing the ENEX to a file was not completed "
                           "successfully"));

            errorDescription.details() = QStringLiteral("Bytes written = ") +
                QString::number(bytes) + QStringLiteral(" while expected ") +
                QString::number(rawEnexData.size());

            QNWARNING("widget:note_editor", errorDescription);
            return false;
        }

        QNDEBUG("widget:note_editor", "Successfully exported the note to ENEX");
        return true;
    }

    QNTRACE(
        "widget:note_editor",
        "Exporting the note to ENEX has been cancelled");

    return false;
}

void NoteEditorWidget::refreshSpecialIcons()
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::refreshSpecialIcons, "
            << "note local id = " << m_noteLocalId);

    setupSpecialIcons();
}

void NoteEditorWidget::dragEnterEvent(QDragEnterEvent * pEvent)
{
    QNTRACE("widget:note_editor", "NoteEditorWidget::dragEnterEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "widget:note_editor",
            "Detected null pointer to "
                << "QDragEnterEvent in NoteEditorWidget's dragEnterEvent");
        return;
    }

    const auto * pMimeData = pEvent->mimeData();
    if (Q_UNLIKELY(!pMimeData)) {
        QNWARNING(
            "widget:note_editor",
            "Null pointer to mime data from drag "
                << "enter event was detected");
        return;
    }

    if (!pMimeData->hasFormat(TAG_MODEL_MIME_TYPE)) {
        QNTRACE("widget:note_editor", "Not tag mime type, skipping");
        return;
    }

    pEvent->acceptProposedAction();
}

void NoteEditorWidget::dragMoveEvent(QDragMoveEvent * pEvent)
{
    QNTRACE("widget:note_editor", "NoteEditorWidget::dragMoveEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "widget:note_editor",
            "Detected null pointer to "
                << "QDropMoveEvent in NoteEditorWidget's dragMoveEvent");
        return;
    }

    const auto * pMimeData = pEvent->mimeData();
    if (Q_UNLIKELY(!pMimeData)) {
        QNWARNING(
            "widget:note_editor",
            "Null pointer to mime data from drag "
                << "move event was detected");
        return;
    }

    if (!pMimeData->hasFormat(TAG_MODEL_MIME_TYPE)) {
        QNTRACE("widget:note_editor", "Not tag mime type, skipping");
        return;
    }

    pEvent->acceptProposedAction();
}

void NoteEditorWidget::dropEvent(QDropEvent * pEvent)
{
    QNTRACE("widget:note_editor", "NoteEditorWidget::dropEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "widget:note_editor",
            "Detected null pointer to QDropEvent "
                << "in NoteEditorWidget's dropEvent");
        return;
    }

    const auto * pMimeData = pEvent->mimeData();
    if (Q_UNLIKELY(!pMimeData)) {
        QNWARNING(
            "widget:note_editor",
            "Null pointer to mime data from drop "
                << "event was detected");
        return;
    }

    if (!pMimeData->hasFormat(TAG_MODEL_MIME_TYPE)) {
        return;
    }

    QByteArray data = qUncompress(pMimeData->data(TAG_MODEL_MIME_TYPE));
    QDataStream in(&data, QIODevice::ReadOnly);

    qint32 type = 0;
    in >> type;

    if (type != static_cast<qint32>(ITagModelItem::Type::Tag)) {
        QNDEBUG(
            "widget:note_editor",
            "Can only drop tag model items of tag "
                << "type onto NoteEditorWidget");
        return;
    }

    TagItem item;
    in >> item;

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        QNDEBUG(
            "widget:note_editor",
            "Can't drop tag onto NoteEditorWidget: "
                << "no note is set to the editor");
        return;
    }

    if (m_pCurrentNote->tagLocalIds().contains(item.localId())) {
        QNDEBUG(
            "widget:note_editor",
            "Note set to the note editor ("
                << m_pCurrentNote->localId()
                << ")is already marked with tag with "
                << "local id " << item.localId());
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "Adding tag with local id " << item.localId()
                                     << " to note with local id "
                                     << m_pCurrentNote->localId());

    m_pCurrentNote->mutableTagLocalIds().push_back(item.localId());

    if (!item.guid().isEmpty()) {
        if (m_pCurrentNote->tagGuids()) {
            m_pCurrentNote->mutableTagGuids()->push_back(item.guid());
        }
        else {
            m_pCurrentNote->setTagGuids(
                QList<qevercloud::Guid>{} << item.guid());
        }
    }

    const QStringList tagLocalIds = m_pCurrentNote->tagLocalIds();

    QStringList tagGuids = m_pCurrentNote->tagGuids().value_or(QStringList{});

    m_pUi->noteEditor->setTagIds(tagLocalIds, tagGuids);
    updateNoteInLocalStorage();

    pEvent->acceptProposedAction();
}

void NoteEditorWidget::closeEvent(QCloseEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "widget:note_editor",
            "Detected null pointer to QCloseEvent "
                << "in NoteEditorWidget's closeEvent");
        return;
    }

    ErrorString errorDescription;
    const auto status = checkAndSaveModifiedNote(errorDescription);

    QNDEBUG(
        "widget:note_editor",
        "Check and save modified note, status: "
            << status << ", error description: " << errorDescription);

    pEvent->accept();
}

bool NoteEditorWidget::eventFilter(QObject * pWatched, QEvent * pEvent)
{
    if (Q_UNLIKELY(!pWatched)) {
        QNWARNING(
            "widget:note_editor",
            "NoteEditorWidget: caught event for "
                << "null watched object in the eventFilter method");
        return true;
    }

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "widget:note_editor",
            "NoteEditorWidget: caught null event "
                << "in the eventFilter method for object "
                << pWatched->objectName() << " of class "
                << pWatched->metaObject()->className());
        return true;
    }

    if (pWatched == m_pUi->noteNameLineEdit) {
        const auto eventType = pEvent->type();
        if (eventType == QEvent::FocusIn) {
            QNDEBUG("widget:note_editor", "Note title editor gained focus");
        }
        else if (eventType == QEvent::FocusOut) {
            QNDEBUG("widget:note_editor", "Note title editor lost focus");
        }
    }
    else if (pWatched == m_pUi->noteEditor) {
        const auto eventType = pEvent->type();
        if (eventType == QEvent::FocusIn) {
            QNDEBUG("widget:note_editor", "Note editor gained focus");
        }
        else if (eventType == QEvent::FocusOut) {
            QNDEBUG("widget:note_editor", "Note editor lost focus");
        }
    }
    else if (pWatched == m_pUi->noteEditor->backend()->widget()) {
        const auto eventType = pEvent->type();
        if (eventType == QEvent::DragEnter) {
            QNTRACE(
                "widget:note_editor",
                "Detected drag enter event for note editor");

            auto * pDragEnterEvent = dynamic_cast<QDragEnterEvent *>(pEvent);
            if (pDragEnterEvent && pDragEnterEvent->mimeData() &&
                pDragEnterEvent->mimeData()->hasFormat(TAG_MODEL_MIME_TYPE))
            {
                QNDEBUG(
                    "widget:note_editor",
                    "Stealing drag enter event with "
                        << "tag data from note editor");
                dragEnterEvent(pDragEnterEvent);
                return true;
            }

            QNTRACE(
                "widget:note_editor",
                "Detected no tag data in the event");
        }
        else if (eventType == QEvent::DragMove) {
            QNTRACE(
                "widget:note_editor",
                "Detected drag move event for note editor");

            auto * pDragMoveEvent = dynamic_cast<QDragMoveEvent *>(pEvent);
            if (pDragMoveEvent && pDragMoveEvent->mimeData() &&
                pDragMoveEvent->mimeData()->hasFormat(TAG_MODEL_MIME_TYPE))
            {
                QNDEBUG(
                    "widget:note_editor",
                    "Stealing drag move event with "
                        << "tag data from note editor");
                dragMoveEvent(pDragMoveEvent);
                return true;
            }

            QNTRACE(
                "widget:note_editor",
                "Detected no tag data in the event");
        }
        else if (eventType == QEvent::Drop) {
            QNTRACE(
                "widget:note_editor",
                "Detected drop event for note "
                    << "editor");

            auto * pDropEvent = dynamic_cast<QDropEvent *>(pEvent);
            if (pDropEvent && pDropEvent->mimeData() &&
                pDropEvent->mimeData()->hasFormat(TAG_MODEL_MIME_TYPE))
            {
                QNDEBUG(
                    "widget:note_editor",
                    "Stealing drop event with tag data from note editor");
                dropEvent(pDropEvent);
                return true;
            }

            QNTRACE(
                "widget:note_editor",
                "Detected no tag data in the event");
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

void NoteEditorWidget::onEditorChooseTextColor(QColor color) // NOLINT
{
    m_pUi->noteEditor->setFontColor(color);
}

void NoteEditorWidget::onEditorChooseBackgroundColor(QColor color) // NOLINT
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

void NoteEditorWidget::onEditorInsertTable(
    int rows, int columns, double width, bool relativeWidth)
{
    rows = std::max(rows, 1);
    columns = std::max(columns, 1);
    width = std::max(width, 1.0);

    if (relativeWidth) {
        m_pUi->noteEditor->insertRelativeWidthTable(rows, columns, width);
    }
    else {
        m_pUi->noteEditor->insertFixedWidthTable(
            rows, columns, static_cast<int>(width));
    }

    QNTRACE(
        "widget:note_editor",
        "Inserted table: rows = "
            << rows << ", columns = " << columns << ", width = " << width
            << ", relative width = " << (relativeWidth ? "true" : "false"));
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
    QNDEBUG("widget:note_editor", "NoteEditorWidget::onSaveNoteAction");

    if (!m_pUi->noteEditor->isModified()) {
        QNDEBUG("widget:note_editor", "Note is not modified");
        m_pUi->saveNotePushButton->setEnabled(false);
        return;
    }

    m_pUi->saveNotePushButton->setEnabled(false);
    m_pUi->noteEditor->saveNoteToLocalStorage();
}

void NoteEditorWidget::onSetUseLimitedFonts(bool useLimitedFonts)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onSetUseLimitedFonts: "
            << (useLimitedFonts ? "true" : "false"));

    const bool currentlyUsingLimitedFonts =
        !(m_pUi->limitedFontComboBox->isHidden());

    if (currentlyUsingLimitedFonts == useLimitedFonts) {
        QNDEBUG(
            "widget:note_editor",
            "The setting has not changed, nothing to do");
        return;
    }

    const QString currentFontFamily =
        (useLimitedFonts ? m_pUi->limitedFontComboBox->currentText()
                         : m_pUi->fontComboBox->currentFont().family());

    if (useLimitedFonts) {
        QObject::disconnect(
            m_pUi->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);

        m_pUi->fontComboBox->setHidden(true);
        m_pUi->fontComboBox->setDisabled(true);

        setupLimitedFontsComboBox(currentFontFamily);
        m_pUi->limitedFontComboBox->setHidden(false);
        m_pUi->limitedFontComboBox->setDisabled(false);
    }
    else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::disconnect(
            m_pUi->limitedFontComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);
#else
        QObject::disconnect(
            m_pUi->limitedFontComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onLimitedFontsComboBoxCurrentIndexChanged(int)));
#endif

        m_pUi->limitedFontComboBox->setHidden(true);
        m_pUi->limitedFontComboBox->setDisabled(true);

        QFont font;
        font.setFamily(currentFontFamily);
        m_pUi->fontComboBox->setFont(font);

        m_pUi->fontComboBox->setHidden(false);
        m_pUi->fontComboBox->setDisabled(false);

        QObject::connect(
            m_pUi->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);
    }
}

void NoteEditorWidget::onNoteTagsListChanged(qevercloud::Note note) // NOLINT
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::onNoteTagsListChanged");

    m_pUi->noteEditor->setTagIds(note.tagLocalIds(), QStringList());

    if (m_pUi->noteEditor->isModified()) {
        m_pUi->noteEditor->saveNoteToLocalStorage();
    }
}

void NoteEditorWidget::onNoteEditorFontColorChanged(QColor color) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNoteEditorFontColorChanged: " << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorBackgroundColorChanged(QColor color) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNoteEditorBackgroundColorChanged: "
            << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorHighlightColorChanged(QColor color) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNoteEditorHighlightColorChanged: "
            << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorHighlightedTextColorChanged(
    QColor color) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNoteEditorHighlightedTextColorChanged: "
            << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorColorsReset()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::onNoteEditorColorsReset");
    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem()
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem");

    auto * pFocusWidget = qApp->focusWidget();
    if (pFocusWidget) {
        QNTRACE(
            "widget:note_editor",
            "Clearing the focus from the widget "
                << "having it currently: " << pFocusWidget);
        pFocusWidget->clearFocus();
    }

    setFocusToEditor();
}

void NoteEditorWidget::onFontComboBoxFontChanged(const QFont & font)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFontComboBoxFontChanged: font family = "
            << font.family());

    if (m_lastFontComboBoxFontFamily == font.family()) {
        QNTRACE("widget:note_editor", "Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = font.family();

    if (!m_pCurrentNote) {
        QNTRACE(
            "widget:note_editor",
            "No note is set to the editor, nothing "
                << "to do");
        return;
    }

    m_pUi->noteEditor->setFont(font);
}

void NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged(
    int fontFamilyIndex)
{
    QString fontFamily =
        m_pUi->limitedFontComboBox->itemText(fontFamilyIndex);

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged: "
            << fontFamily);

    if (fontFamily.trimmed().isEmpty()) {
        QNTRACE(
            "widget:note_editor",
            "Font family is empty, ignoring this update");
        return;
    }

    removeSurrondingApostrophes(fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE("widget:note_editor", "Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    if (!m_pCurrentNote) {
        QNTRACE(
            "widget:note_editor",
            "No note is set to the editor, nothing "
                << "to do");
        return;
    }

    QFont font;
    font.setFamily(fontFamily);
    m_pUi->noteEditor->setFont(font);
}

void NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged(int index)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged: index = "
            << index);

    if (index == m_lastFontSizeComboBoxIndex) {
        QNTRACE("widget:note_editor", "Font size hasn't changed");
        return;
    }

    m_lastFontSizeComboBoxIndex = index;

    if (!m_pCurrentNote) {
        QNTRACE(
            "widget:note_editor",
            "No note is set to the editor, nothing "
                << "to do");
        return;
    }

    if (m_lastFontSizeComboBoxIndex < 0) {
        QNTRACE(
            "widget:note_editor",
            "Font size combo box index is negative, "
                << "won't do anything");
        return;
    }

    bool conversionResult = false;
    const auto data = m_pUi->fontSizeComboBox->itemData(index);
    const int fontSize = data.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString error(
            QT_TR_NOOP("Can't process the change of font size combo box index: "
                       "can't convert combo box item data to int"));
        QNWARNING("widget:note_editor", error << ": " << data);
        Q_EMIT notifyError(error);
        return;
    }

    m_pUi->noteEditor->setFontHeight(fontSize);
}

void NoteEditorWidget::onNoteSavedToLocalStorage(QString noteLocalId) // NOLINT
{
    if (Q_UNLIKELY(
            !m_pCurrentNote || (m_pCurrentNote->localId() != noteLocalId))) {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNoteSavedToLocalStorage: note local id = "
            << noteLocalId);

    Q_EMIT noteSavedInLocalStorage();
}

void NoteEditorWidget::onFailedToSaveNoteToLocalStorage(
    ErrorString errorDescription, QString noteLocalId) // NOLINT
{
    if (Q_UNLIKELY(
            !m_pCurrentNote || (m_pCurrentNote->localId() != noteLocalId))) {
        return;
    }

    QNWARNING(
        "widget:note_editor",
        "NoteEditorWidget::onFailedToSaveNoteToLocalStorage: "
            << "note local id = " << errorDescription
            << ", error description: " << errorDescription);

    m_pUi->saveNotePushButton->setEnabled(true);

    ErrorString error(QT_TR_NOOP("Failed to save the updated note"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);

    // NOTE: not clearing out the unsaved stuff because it may be of value
    // to the user

    Q_EMIT noteSaveInLocalStorageFailed();
}

void NoteEditorWidget::onUpdateNoteComplete(
    qevercloud::Note note, // NOLINT
    LocalStorageManager::UpdateNoteOptions options,
    QUuid requestId)
{
    if (Q_UNLIKELY(
            !m_pCurrentNote || (m_pCurrentNote->localId() != note.localId())))
    {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onUpdateNoteComplete: "
            << "note local id = " << note.localId()
            << ", request id = " << requestId << ", update resource metadata = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                    ? "true"
                    : "false")
            << ", update resource binary data = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::
                     UpdateResourceBinaryData)
                    ? "true"
                    : "false")
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                    ? "true"
                    : "false"));

    const bool updateResources =
        (options &
         LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata) &&
        (options &
         LocalStorageManager::UpdateNoteOption::UpdateResourceBinaryData);

    const bool updateTags =
        (options & LocalStorageManager::UpdateNoteOption::UpdateTags);

    QList<qevercloud::Resource> backupResources;
    if (!updateResources) {
        backupResources = m_pCurrentNote->resources().value_or(
            QList<qevercloud::Resource>{});
    }

    QStringList backupTagLocalIds;
    if (!updateTags) {
        backupTagLocalIds = m_pCurrentNote->tagLocalIds();
    }

    QStringList backupTagGuids;
    bool hadTagGuids = m_pCurrentNote->tagGuids().has_value();
    if (!updateTags && hadTagGuids) {
        backupTagGuids = *m_pCurrentNote->tagGuids();
    }

    QString backupTitle;
    if (m_noteTitleIsEdited) {
        backupTitle = m_pCurrentNote->title().value_or(QString{});
    }

    *m_pCurrentNote = note;

    if (!updateResources) {
        m_pCurrentNote->setResources(backupResources);
    }

    if (!updateTags) {
        m_pCurrentNote->setTagLocalIds(backupTagLocalIds);
        m_pCurrentNote->setTagGuids(backupTagGuids);
    }

    if (m_noteTitleIsEdited) {
        m_pCurrentNote->setTitle(
            backupTitle.isEmpty()
            ? std::nullopt
            : std::make_optional(backupTitle));
    }

    if (note.deleted()) {
        QNINFO(
            "widget:note_editor",
            "The note loaded into the editor was deleted: " << *m_pCurrentNote);
        Q_EMIT invalidated();
        return;
    }

    if (Q_UNLIKELY(!m_pCurrentNotebook)) {
        QNDEBUG(
            "widget:note_editor",
            "Current notebook is null - a bit unexpected at this point");

        if (!m_findCurrentNotebookRequestId.isNull()) {
            QNDEBUG(
                "widget:note_editor",
                "The request to find the current "
                    << "notebook is still active, waiting for it to finish");
            return;
        }

        const qevercloud::Notebook * pCachedNotebook = nullptr;
        if (!m_pCurrentNote->notebookLocalId().isEmpty()) {
            const QString & notebookLocalId =
                m_pCurrentNote->notebookLocalId();

            pCachedNotebook = m_notebookCache.get(notebookLocalId);
        }

        if (pCachedNotebook) {
            m_pCurrentNotebook = std::make_unique<qevercloud::Notebook>(
                *pCachedNotebook);
        }
        else {
            if (Q_UNLIKELY(
                    m_pCurrentNote->notebookLocalId().isEmpty() &&
                    !m_pCurrentNote->notebookGuid()))
            {
                ErrorString error(
                    QT_TR_NOOP("Note has neither notebook local "
                               "uid nor notebook guid"));
                if (note.title()) {
                    error.details() = *note.title();
                }
                else {
                    error.details() = m_pCurrentNote->localId();
                }

                QNWARNING(
                    "widget:note_editor",
                    error << ", note: " << *m_pCurrentNote);

                Q_EMIT notifyError(error);
                clear();
                return;
            }

            m_findCurrentNotebookRequestId = QUuid::createUuid();
            qevercloud::Notebook dummy;
            if (!m_pCurrentNote->notebookLocalId().isEmpty()) {
                dummy.setLocalId(m_pCurrentNote->notebookLocalId());
            }
            else {
                dummy.setLocalId(QString{});
                dummy.setGuid(m_pCurrentNote->notebookGuid());
            }

            QNTRACE(
                "widget:note_editor",
                "Emitting the request to find "
                    << "the current notebook: " << dummy
                    << "\nRequest id = " << m_findCurrentNotebookRequestId);

            Q_EMIT findNotebook(dummy, m_findCurrentNotebookRequestId);
            return;
        }
    }

    setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);
}

void NoteEditorWidget::onFindNoteComplete(
    qevercloud::Note note, // NOLINT
    LocalStorageManager::GetNoteOptions options, QUuid requestId)
{
    const auto it = m_noteLinkInfoByFindNoteRequestIds.find(requestId);
    if (it == m_noteLinkInfoByFindNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindNoteComplete: "
            << "request id = " << requestId << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false"));

    QNTRACE("widget:note_editor", "Note: " << note);

    const auto & noteLinkInfo = it.value();

    QString titleOrPreview;
    if (note.title()) {
        titleOrPreview = *note.title();
    }
    else if (note.content()) {
        titleOrPreview = noteContentToPlainText(*note.content());
        titleOrPreview.truncate(140);
    }

    QNTRACE(
        "widget:note_editor",
        "Emitting the request to insert the in-app "
            << "note link: user id = " << noteLinkInfo.m_userId
            << ", shard id = " << noteLinkInfo.m_shardId
            << ", note guid = " << noteLinkInfo.m_noteGuid
            << ", title or preview = " << titleOrPreview);

    Q_EMIT insertInAppNoteLink(
        noteLinkInfo.m_userId, noteLinkInfo.m_shardId, noteLinkInfo.m_noteGuid,
        titleOrPreview);

    Q_UNUSED(m_noteLinkInfoByFindNoteRequestIds.erase(it))
}

void NoteEditorWidget::onFindNoteFailed(
    qevercloud::Note note, // NOLINT
    LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId) // NOLINT
{
    const auto it = m_noteLinkInfoByFindNoteRequestIds.find(requestId);
    if (it == m_noteLinkInfoByFindNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindNoteFailed: "
            << "request id = " << requestId << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false")
            << ", error description: " << errorDescription);

    QNTRACE("widget:note_editor", "Note: " << note);

    const auto & noteLinkInfo = it.value();

    QNTRACE(
        "widget:note_editor",
        "Emitting the request to insert the in-app "
            << "note link without the link text as the note was not found: "
            << "user id = " << noteLinkInfo.m_userId
            << ", shard id = " << noteLinkInfo.m_shardId
            << ", note guid = " << noteLinkInfo.m_noteGuid);

    Q_EMIT insertInAppNoteLink(
        noteLinkInfo.m_userId, noteLinkInfo.m_shardId, noteLinkInfo.m_noteGuid,
        QString());

    Q_UNUSED(m_noteLinkInfoByFindNoteRequestIds.erase(it))
}

void NoteEditorWidget::onExpungeNoteComplete(
    qevercloud::Note note, QUuid requestId) // NOLINT
{
    if (!m_pCurrentNote || (m_pCurrentNote->localId() != note.localId())) {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onExpungeNoteComplete: "
            << "note local id = " << note.localId()
            << ", request id = " << requestId);

    m_currentNoteWasExpunged = true;

    // TODO: ideally should allow the choice to either re-save the note or to
    // drop it

    QNINFO(
        "widget:note_editor",
        "The note loaded into the editor was expunged "
            << "from the local storage: " << *m_pCurrentNote);

    Q_EMIT invalidated();
}

void NoteEditorWidget::onAddResourceComplete(
    qevercloud::Resource resource, QUuid requestId) // NOLINT
{
    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        return;
    }

    const auto & resourceNoteLocalId = resource.noteLocalId();
    if (resourceNoteLocalId.isEmpty() ||
        (resourceNoteLocalId != m_pCurrentNote->localId()))
    {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onAddResourceComplete: "
            << "request id = " << requestId << ", resource: " << resource);

    if (m_pCurrentNote->resources() && !m_pCurrentNote->resources()->isEmpty())
    {
        const auto resources = *m_pCurrentNote->resources();
        for (auto it = resources.constBegin(), end = resources.constEnd();
             it != end; ++it)
        {
            const auto & currentResource = *it;
            if (currentResource.localId() == resource.localId()) {
                QNDEBUG(
                    "widget:note_editor",
                    "Found the added resource already belonging to the note");
                return;
            }
        }
    }

    // Haven't found the added resource within the note's resources, need to add
    // it and update the note editor
    if (m_pCurrentNote->resources()) {
        m_pCurrentNote->mutableResources()->push_back(resource);
    }
    else {
        m_pCurrentNote->setResources(QList<qevercloud::Resource>{} << resource);
    }
}

void NoteEditorWidget::onUpdateResourceComplete(
    qevercloud::Resource resource, QUuid requestId) // NOLINT
{
    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        return;
    }

    const auto & resourceNoteLocalId = resource.noteLocalId();
    if (resourceNoteLocalId.isEmpty() ||
        (resourceNoteLocalId != m_pCurrentNote->localId()))
    {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onUpdateResourceComplete: "
            << "request id = " << requestId << ", resource: " << resource);

    // TODO: ideally should allow the choice to either leave the current version
    // of the note or to reload it

    if (m_pCurrentNote->resources()) {
        m_pCurrentNote->mutableResources()->push_back(resource);
    }
    else {
        m_pCurrentNote->setResources(QList<qevercloud::Resource>{} << resource);
    }
}

void NoteEditorWidget::onExpungeResourceComplete(
    qevercloud::Resource resource, QUuid requestId)
{
    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        return;
    }

    const auto & resourceNoteLocalId = resource.noteLocalId();
    if (resourceNoteLocalId.isEmpty() ||
        (resourceNoteLocalId != m_pCurrentNote->localId()))
    {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onExpungeResourceComplete: request id = "
            << requestId << ", resource: " << resource);

    // TODO: ideally should allow the choice to either leave the current version
    // of the note or to reload it

    if (!m_pCurrentNote->resources() || m_pCurrentNote->resources()->isEmpty())
    {
        return;
    }

    auto it = std::find_if(
        m_pCurrentNote->mutableResources()->begin(),
        m_pCurrentNote->mutableResources()->end(),
        [&](const qevercloud::Resource & r)
        {
            return r.localId() == resource.localId();
        });
    if (it != m_pCurrentNote->mutableResources()->end()) {
        m_pCurrentNote->mutableResources()->erase(it);
    }
}

void NoteEditorWidget::onUpdateNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    if (!m_pCurrentNote || !m_pCurrentNotebook ||
        (m_pCurrentNotebook->localId() != notebook.localId()))
    {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onUpdateNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    setNoteAndNotebook(*m_pCurrentNote, notebook);
}

void NoteEditorWidget::onExpungeNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    if (!m_pCurrentNotebook ||
        (m_pCurrentNotebook->localId() != notebook.localId()))
    {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onExpungeNotebookComplete: notebook = "
            << notebook << "\nRequest id = " << requestId);

    QNINFO(
        "widget:note_editor",
        "The notebook containing the note loaded into "
            << "the editor was expunged from the local storage: "
            << *m_pCurrentNotebook);

    clear();
    Q_EMIT invalidated();
}

void NoteEditorWidget::onFindNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    if (requestId != m_findCurrentNotebookRequestId) {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindNotebookComplete: "
            << "request id = " << requestId << ", notebook: " << notebook);

    m_findCurrentNotebookRequestId = QUuid();

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        QNDEBUG(
            "widget:note_editor",
            "Can't process the update of "
                << "the notebook: no current note is set");
        clear();
        return;
    }

    m_pCurrentNotebook = std::make_unique<qevercloud::Notebook>(notebook);
    setNoteAndNotebook(*m_pCurrentNote, *m_pCurrentNotebook);

    QNTRACE(
        "widget:note_editor",
        "Emitting resolved signal, note local id = " << m_noteLocalId);

    Q_EMIT resolved();
}

void NoteEditorWidget::onFindNotebookFailed(
    qevercloud::Notebook notebook, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    if (requestId != m_findCurrentNotebookRequestId) {
        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindNotebookFailed: "
            << "request id = " << requestId << ", notebook: " << notebook
            << "\nError description = " << errorDescription);

    m_findCurrentNotebookRequestId = QUuid();
    clear();

    Q_EMIT notifyError(
        ErrorString(QT_TR_NOOP("Can't find the note attempted "
                               "to be selected in the note editor")));
}

void NoteEditorWidget::onNoteTitleEdited(const QString & noteTitle)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onNoteTitleEdited: " << noteTitle);

    m_noteTitleIsEdited = true;
    m_noteTitleHasBeenEdited = true;
}

void NoteEditorWidget::onNoteEditorModified()
{
    QNTRACE("widget:note_editor", "NoteEditorWidget::onNoteEditorModified");

    m_noteHasBeenModified = true;

    if (!m_pUi->noteEditor->isNoteLoaded()) {
        QNTRACE("widget:note_editor", "The note is still being loaded");
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

    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNoteTitleUpdated: " << noteTitle);

    m_noteTitleIsEdited = false;
    m_noteTitleHasBeenEdited = true;

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        // That shouldn't really happen in normal circumstances but it could
        // in theory be some old event which has reached this object after
        // the note has already been cleaned up
        QNDEBUG(
            "widget:note_editor",
            "No current note in the note editor "
                << "widget! Ignoring the note title update");
        return;
    }

    if (!m_pCurrentNote->title() && noteTitle.isEmpty()) {
        QNDEBUG(
            "widget:note_editor",
            "Note's title is still empty, nothing to do");
        return;
    }

    if (m_pCurrentNote->title() && (*m_pCurrentNote->title() == noteTitle)) {
        QNDEBUG(
            "widget:note_editor",
            "Note's title hasn't changed, nothing to do");
        return;
    }

    ErrorString error;
    if (!checkNoteTitle(noteTitle, error)) {
        QPoint targetPoint(0, m_pUi->noteNameLineEdit->height());
        targetPoint = m_pUi->noteNameLineEdit->mapToGlobal(targetPoint);
        QToolTip::showText(targetPoint, error.localizedString());
        return;
    }

    m_pCurrentNote->setTitle(noteTitle);
    m_isNewNote = false;

    m_pUi->noteEditor->setNoteTitle(noteTitle);

    Q_EMIT titleOrPreviewChanged(titleOrPreview());

    updateNoteInLocalStorage();
}

void NoteEditorWidget::onEditorNoteUpdate(qevercloud::Note note) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onEditorNoteUpdate: "
            << "note local id = " << note.localId());

    QNTRACE("widget:note_editor", "Note: " << note);

    if (Q_UNLIKELY(!m_pCurrentNote)) {
        // That shouldn't really happen in normal circumstances but it could
        // in theory be some old event which has reached this object after
        // the note has already been cleaned up
        QNDEBUG(
            "widget:note_editor",
            "No current note in the note editor "
                << "widget! Ignoring the update from the note editor");
        return;
    }

    QString noteTitle = m_pCurrentNote->title().value_or(QString{});
    *m_pCurrentNote = note;
    m_pCurrentNote->setTitle(std::move(noteTitle));

    updateNoteInLocalStorage();
}

void NoteEditorWidget::onEditorNoteUpdateFailed(ErrorString error) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onEditorNoteUpdateFailed: " << error);

    Q_EMIT notifyError(error);
    Q_EMIT conversionToNoteFailed();
}

void NoteEditorWidget::onEditorInAppLinkPasteRequested(
    QString url, QString userId, QString shardId, QString noteGuid) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onEditorInAppLinkPasteRequested: url = "
            << url << ", user id = " << userId << ", shard id = " << shardId
            << ", note guid = " << noteGuid);

    NoteLinkInfo noteLinkInfo;
    noteLinkInfo.m_userId = userId;
    noteLinkInfo.m_shardId = shardId;
    noteLinkInfo.m_noteGuid = noteGuid;

    QUuid requestId = QUuid::createUuid();
    m_noteLinkInfoByFindNoteRequestIds[requestId] = noteLinkInfo;

    qevercloud::Note dummyNote;
    dummyNote.setLocalId(QString{});
    dummyNote.setGuid(noteGuid);

    QNTRACE(
        "widget:note_editor",
        "Emitting the request to find note by guid "
            << "for the purpose of in-app note link insertion: request id = "
            << requestId << ", note guid = " << noteGuid);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    LocalStorageManager::GetNoteOptions options;
#else
    LocalStorageManager::GetNoteOptions options(0); // NOLINT
#endif

    Q_EMIT findNote(dummyNote, options, requestId);
}

void NoteEditorWidget::onEditorTextBoldStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextBoldStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->fontBoldPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextItalicStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextItalicStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->fontItalicPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextUnderlineStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextUnderlineStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->fontUnderlinePushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextStrikethroughStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextStrikethroughStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->fontStrikethroughPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextAlignLeftStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextAlignLeftStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->formatJustifyLeftPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignCenterStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextAlignCenterStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->formatJustifyCenterPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignRightStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextAlignRightStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->formatJustifyRightPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignFullStateChanged(bool state)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextAlignFullStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->formatJustifyFullPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideOrderedListStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextInsideOrderedListStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->formatListOrderedPushButton->setChecked(state);

    if (state) {
        m_pUi->formatListUnorderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->formatListUnorderedPushButton->setChecked(state);

    if (state) {
        m_pUi->formatListOrderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideTableStateChanged(bool state)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextInsideTableStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_pUi->insertTableToolButton->setEnabled(!state);
}

void NoteEditorWidget::onEditorTextFontFamilyChanged(QString fontFamily)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextFontFamilyChanged: " << fontFamily);

    removeSurrondingApostrophes(fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE("widget:note_editor", "Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    const QFont currentFont{fontFamily};

    if (!(m_pUi->limitedFontComboBox->isHidden())) {
        if (Q_UNLIKELY(!m_pLimitedFontsListModel)) {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't process the change of font: internal error, "
                           "no limited fonts list model is set up"));

            QNWARNING("widget:note_editor", errorDescription);
            Q_EMIT notifyError(errorDescription);
            return;
        }

        QStringList fontNames = m_pLimitedFontsListModel->stringList();
        int index = fontNames.indexOf(fontFamily);
        if (index < 0) {
            QNDEBUG(
                "widget:note_editor",
                "Could not find font name "
                    << fontFamily << " within available font names: "
                    << fontNames.join(QStringLiteral(", "))
                    << ", will try to match the font family \"approximately\"");

            for (auto it = fontNames.constBegin(), end = fontNames.constEnd();
                 it != end; ++it)
            {
                const QString & currentFontFamily = *it;
                if (fontFamily.contains(
                        currentFontFamily, Qt::CaseInsensitive) ||
                    currentFontFamily.contains(fontFamily, Qt::CaseInsensitive))
                {
                    QNDEBUG(
                        "widget:note_editor",
                        "Note editor's font family "
                            << fontFamily << " appears to correspond to "
                            << currentFontFamily);

                    index = static_cast<int>(
                        std::distance(fontNames.constBegin(), it));

                    break;
                }
            }
        }

        if (index < 0) {
            QNDEBUG(
                "widget:note_editor",
                "Could not find neither exact nor "
                    << "approximate match for the font name " << fontFamily
                    << " within available font names: "
                    << fontNames.join(QStringLiteral(", "))
                    << ", will add the missing font name to the list");

            fontNames << fontFamily;
            fontNames.sort(Qt::CaseInsensitive);

            const auto it = std::lower_bound(
                fontNames.constBegin(), fontNames.constEnd(), fontFamily);

            if ((it != fontNames.constEnd()) && (*it == fontFamily)) {
                index =
                    static_cast<int>(std::distance(fontNames.constBegin(), it));
            }
            else {
                index = fontNames.indexOf(fontFamily);
            }

            if (Q_UNLIKELY(index < 0)) {
                QNWARNING(
                    "widget:note_editor",
                    "Something is weird: can't "
                        << "find the just added font " << fontFamily
                        << " within the list of available fonts where it "
                        << "should have been just added: "
                        << fontNames.join(QStringLiteral(", ")));

                index = 0;
            }

            m_pLimitedFontsListModel->setStringList(fontNames);
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::disconnect(
            m_pUi->limitedFontComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);
#else
        QObject::disconnect(
            m_pUi->limitedFontComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onLimitedFontsComboBoxCurrentIndexChanged(int)));
#endif

        m_pUi->limitedFontComboBox->setCurrentIndex(index);

        QNTRACE(
            "widget:note_editor",
            "Set limited font combo box index to "
                << index << " corresponding to font family " << fontFamily);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::connect(
            m_pUi->limitedFontComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);
#else
        QObject::connect(
            m_pUi->limitedFontComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onLimitedFontsComboBoxCurrentIndexChanged(int)));
#endif
    }
    else {
        QObject::disconnect(
            m_pUi->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);

        m_pUi->fontComboBox->setCurrentFont(currentFont);

        QNTRACE(
            "widget:note_editor",
            "Font family from combo box: "
                << m_pUi->fontComboBox->currentFont().family()
                << ", font family set by QFont's constructor from it: "
                << currentFont.family());

        QObject::connect(
            m_pUi->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);
    }

    setupFontSizesForFont(currentFont);
}

void NoteEditorWidget::onEditorTextFontSizeChanged(int fontSize)
{
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextFontSizeChanged: " << fontSize);

    if (m_lastSuggestedFontSize == fontSize) {
        QNTRACE(
            "widget:note_editor",
            "This font size has already been "
                << "suggested previously");
        return;
    }

    m_lastSuggestedFontSize = fontSize;

    const int fontSizeIndex =
        m_pUi->fontSizeComboBox->findData(QVariant(fontSize), Qt::UserRole);

    if (fontSizeIndex >= 0) {
        m_lastFontSizeComboBoxIndex = fontSizeIndex;
        m_lastActualFontSize = fontSize;

        if (m_pUi->fontSizeComboBox->currentIndex() != fontSizeIndex) {
            m_pUi->fontSizeComboBox->setCurrentIndex(fontSizeIndex);

            QNTRACE(
                "widget:note_editor",
                "fontSizeComboBox: set current index "
                    << "to " << fontSizeIndex
                    << ", found font size = " << QVariant(fontSize));
        }

        return;
    }

    QNDEBUG(
        "widget:note_editor",
        "Can't find font size "
            << fontSize
            << " within those listed in font size combobox, will try to choose "
            << "the closest one instead");

    const int numFontSizes = m_pUi->fontSizeComboBox->count();
    int currentSmallestDiscrepancy = 1e5;
    int currentClosestIndex = -1;
    int currentClosestFontSize = -1;

    for (int i = 0; i < numFontSizes; ++i) {
        auto value = m_pUi->fontSizeComboBox->itemData(i, Qt::UserRole);
        bool conversionResult = false;
        const int valueInt = value.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING(
                "widget:note_editor",
                "Can't convert value from font "
                    << "size combo box to int: " << value);
            continue;
        }

        const int discrepancy = abs(valueInt - fontSize);
        if (currentSmallestDiscrepancy > discrepancy) {
            currentSmallestDiscrepancy = discrepancy;
            currentClosestIndex = i;
            currentClosestFontSize = valueInt;
            QNTRACE(
                "widget:note_editor",
                "Updated current closest index to "
                    << i << ": font size = " << valueInt);
        }
    }

    if (currentClosestIndex < 0) {
        QNDEBUG(
            "widget:note_editor",
            "Couldn't find closest font size to " << fontSize);
        return;
    }

    QNTRACE(
        "widget:note_editor",
        "Found closest current font size: "
            << currentClosestFontSize << ", index = " << currentClosestIndex);

    if ((m_lastFontSizeComboBoxIndex == currentClosestIndex) &&
        (m_lastActualFontSize == currentClosestFontSize))
    {
        QNTRACE(
            "widget:note_editor",
            "Neither the font size nor its index "
                << "within the font combo box have changed");
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
    QNTRACE(
        "widget:note_editor",
        "NoteEditorWidget::onEditorTextInsertTableDialogRequested");

    auto pTableSettingsDialog = std::make_unique<TableSettingsDialog>(this);
    if (pTableSettingsDialog->exec() == QDialog::Rejected) {
        QNTRACE(
            "widget:note_editor",
            "Returned from TableSettingsDialog::exec: rejected");
        return;
    }

    QNTRACE(
        "widget:note_editor",
        "Returned from TableSettingsDialog::exec: "
            << "accepted");

    const int numRows = pTableSettingsDialog->numRows();
    const int numColumns = pTableSettingsDialog->numColumns();
    const double tableWidth = pTableSettingsDialog->tableWidth();
    const bool relativeWidth = pTableSettingsDialog->relativeWidth();

    if (relativeWidth) {
        m_pUi->noteEditor->insertRelativeWidthTable(
            numRows, numColumns, tableWidth);
    }
    else {
        m_pUi->noteEditor->insertFixedWidthTable(
            numRows, numColumns, static_cast<int>(tableWidth));
    }

    m_pUi->noteEditor->setFocus();
}

void NoteEditorWidget::onEditorSpellCheckerNotReady()
{
    QNDEBUG(
        "widget:note_editor", "NoteEditorWidget::onEditorSpellCheckerNotReady");

    m_pendingEditorSpellChecker = true;

    Q_EMIT notifyError(ErrorString(
        QT_TR_NOOP("Spell checker is loading dictionaries, please wait")));
}

void NoteEditorWidget::onEditorSpellCheckerReady()
{
    QNDEBUG(
        "widget:note_editor", "NoteEditorWidget::onEditorSpellCheckerReady");

    if (!m_pendingEditorSpellChecker) {
        return;
    }

    m_pendingEditorSpellChecker = false;

    // Send the empty message to remove the previous one about the non-ready
    // spell checker
    Q_EMIT notifyError(ErrorString());
}

void NoteEditorWidget::onEditorHtmlUpdate(QString html) // NOLINT
{
    QNTRACE("widget:note_editor", "NoteEditorWidget::onEditorHtmlUpdate");

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
    QNDEBUG("widget:note_editor", "NoteEditorWidget::onFindInsideNoteAction");

    if (m_pUi->findAndReplaceWidget->isHidden()) {
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

    onFindNextInsideNote(
        m_pUi->findAndReplaceWidget->textToFind(),
        m_pUi->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onFindPreviousInsideNoteAction()
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindPreviousInsideNoteAction");

    if (m_pUi->findAndReplaceWidget->isHidden()) {
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

    onFindPreviousInsideNote(
        m_pUi->findAndReplaceWidget->textToFind(),
        m_pUi->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onReplaceInsideNoteAction()
{
    QNDEBUG(
        "widget:note_editor", "NoteEditorWidget::onReplaceInsideNoteAction");

    if (m_pUi->findAndReplaceWidget->isHidden() ||
        !m_pUi->findAndReplaceWidget->replaceEnabled())
    {
        QNTRACE(
            "widget:note_editor",
            "At least the replacement part of find "
                << "and replace widget is hidden, will only show it and do "
                << "nothing else");

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

    onReplaceInsideNote(
        m_pUi->findAndReplaceWidget->textToFind(),
        m_pUi->findAndReplaceWidget->replacementText(),
        m_pUi->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage(
    qevercloud::Note note, qevercloud::Notebook notebook) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage");

    QNTRACE(
        "widget:note_editor", "Note: " << note << "\nNotebook: " << notebook);

    QObject::disconnect(
        m_pUi->noteEditor, &NoteEditor::noteAndNotebookFoundInLocalStorage,
        this, &NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage);

    QObject::disconnect(
        m_pUi->noteEditor, &NoteEditor::noteNotFound, this,
        &NoteEditorWidget::onNoteNotFoundInLocalStorage);

    setNoteAndNotebook(note, notebook);

    QNTRACE(
        "widget:note_editor",
        "Emitting resolved signal, note local id = " << m_noteLocalId);

    Q_EMIT resolved();
}

void NoteEditorWidget::onNoteNotFoundInLocalStorage(
    QString noteLocalId) // NOLINT
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onNoteNotFoundInLocalStorage: note local id = "
            << noteLocalId);

    clear();

    ErrorString errorDescription(
        QT_TR_NOOP("Can't display note: either note or its notebook were not "
                   "found within the local storage"));

    QNWARNING("widget:note_editor", errorDescription);
    Q_EMIT notifyError(errorDescription);
}

void NoteEditorWidget::onFindAndReplaceWidgetClosed()
{
    QNDEBUG(
        "widget:note_editor", "NoteEditorWidget::onFindAndReplaceWidgetClosed");

    onFindNextInsideNote(QString(), false);
}

void NoteEditorWidget::onTextToFindInsideNoteEdited(const QString & textToFind)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onTextToFindInsideNoteEdited: " << textToFind);

    bool matchCase = m_pUi->findAndReplaceWidget->matchCase();
    onFindNextInsideNote(textToFind, matchCase);
}

#define CHECK_FIND_AND_REPLACE_WIDGET_STATE()                                  \
    if (Q_UNLIKELY(m_pUi->findAndReplaceWidget->isHidden())) {                 \
        QNTRACE(                                                               \
            "widget:note_editor",                                              \
            "Find and replace widget is not shown, "                           \
                << "nothing to do");                                           \
        return;                                                                \
    }

void NoteEditorWidget::onFindNextInsideNote(
    const QString & textToFind, const bool matchCase)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindNextInsideNote: "
            << "text to find = " << textToFind
            << ", match case = " << (matchCase ? "true" : "false"));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->noteEditor->findNext(textToFind, matchCase);
}

void NoteEditorWidget::onFindPreviousInsideNote(
    const QString & textToFind, const bool matchCase)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindPreviousInsideNote: "
            << "text to find = " << textToFind
            << ", match case = " << (matchCase ? "true" : "false"));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->noteEditor->findPrevious(textToFind, matchCase);
}

void NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged(
    const bool matchCase)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged: "
            << "match case = " << (matchCase ? "true" : "false"));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()

    QString textToFind = m_pUi->findAndReplaceWidget->textToFind();
    m_pUi->noteEditor->findNext(textToFind, matchCase);
}

void NoteEditorWidget::onReplaceInsideNote(
    const QString & textToReplace, const QString & replacementText,
    const bool matchCase)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onReplaceInsideNote: "
            << "text to replace = " << textToReplace
            << ", replacement text = " << replacementText
            << ", match case = " << (matchCase ? "true" : "false"));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->findAndReplaceWidget->setReplaceEnabled(true);

    m_pUi->noteEditor->replace(textToReplace, replacementText, matchCase);
}

void NoteEditorWidget::onReplaceAllInsideNote(
    const QString & textToReplace, const QString & replacementText,
    const bool matchCase)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onReplaceAllInsideNote: "
            << "text to replace = " << textToReplace
            << ", replacement text = " << replacementText
            << ", match case = " << (matchCase ? "true" : "false"));

    CHECK_FIND_AND_REPLACE_WIDGET_STATE()
    m_pUi->findAndReplaceWidget->setReplaceEnabled(true);

    m_pUi->noteEditor->replaceAll(textToReplace, replacementText, matchCase);
}

#undef CHECK_FIND_AND_REPLACE_WIDGET_STATE

void NoteEditorWidget::updateNoteInLocalStorage()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::updateNoteInLocalStorage");

    if (!m_pCurrentNote) {
        QNDEBUG("widget:note_editor", "No note is set to the editor");
        return;
    }

    m_pUi->saveNotePushButton->setEnabled(false);

    m_isNewNote = false;

    m_pUi->noteEditor->saveNoteToLocalStorage();
}

void NoteEditorWidget::onPrintNoteButtonPressed()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::onPrintNoteButtonPressed");

    ErrorString errorDescription;
    bool res = printNote(errorDescription);
    if (!res) {
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteEditorWidget::onExportNoteToPdfButtonPressed()
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onExportNoteToPdfButtonPressed");

    ErrorString errorDescription;
    bool res = exportNoteToPdf(errorDescription);
    if (!res) {
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteEditorWidget::onExportNoteToEnexButtonPressed()
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::onExportNoteToEnexButtonPressed");

    ErrorString errorDescription;
    bool res = exportNoteToEnex(errorDescription);
    if (!res) {
        if (errorDescription.isEmpty()) {
            return;
        }

        Q_EMIT notifyError(errorDescription);
    }
}

void NoteEditorWidget::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::createConnections");

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(
        this, &NoteEditorWidget::findNote, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNoteRequest);

    QObject::connect(
        this, &NoteEditorWidget::findNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNotebookRequest);

    // Local signals to note editor's slots
    QObject::connect(
        this, &NoteEditorWidget::insertInAppNoteLink, m_pUi->noteEditor,
        &NoteEditor::insertInAppNoteLink);

    // localStorageManagerAsync's signals to local slots
    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteComplete, this,
        &NoteEditorWidget::onUpdateNoteComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findNoteComplete,
        this, &NoteEditorWidget::onFindNoteComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findNoteFailed,
        this, &NoteEditorWidget::onFindNoteFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNoteComplete, this,
        &NoteEditorWidget::onExpungeNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addResourceComplete, this,
        &NoteEditorWidget::onAddResourceComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateResourceComplete, this,
        &NoteEditorWidget::onUpdateResourceComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeResourceComplete, this,
        &NoteEditorWidget::onExpungeResourceComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookComplete, this,
        &NoteEditorWidget::onUpdateNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookComplete, this,
        &NoteEditorWidget::onFindNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookFailed, this,
        &NoteEditorWidget::onFindNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &NoteEditorWidget::onExpungeNotebookComplete);

    // Connect to font sizes combobox signals
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->fontSizeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged,
        Qt::UniqueConnection);
#else
    QObject::connect(
        m_pUi->fontSizeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onFontSizesComboBoxCurrentIndexChanged(int)),
        Qt::UniqueConnection);
#endif

    // Connect to note tags widget's signals
    QObject::connect(
        m_pUi->tagNameLabelsContainer, &NoteTagsWidget::noteTagsListChanged,
        this, &NoteEditorWidget::onNoteTagsListChanged);

    QObject::connect(
        m_pUi->tagNameLabelsContainer,
        &NoteTagsWidget::newTagLineEditReceivedFocusFromWindowSystem, this,
        &NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem);

    // Connect to note title updates
    QObject::connect(
        m_pUi->noteNameLineEdit, &QLineEdit::textEdited, this,
        &NoteEditorWidget::onNoteTitleEdited);

    QObject::connect(
        m_pUi->noteNameLineEdit, &QLineEdit::editingFinished, this,
        &NoteEditorWidget::onNoteTitleUpdated);

    // Connect note editor's signals to local signals and slots
    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::inAppNoteLinkClicked, this,
        &NoteEditorWidget::inAppNoteLinkClicked);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::inAppNoteLinkPasteRequested, this,
        &NoteEditorWidget::onEditorInAppLinkPasteRequested);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::convertedToNote, this,
        &NoteEditorWidget::onEditorNoteUpdate);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::cantConvertToNote, this,
        &NoteEditorWidget::onEditorNoteUpdateFailed);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::noteSavedToLocalStorage, this,
        &NoteEditorWidget::onNoteSavedToLocalStorage);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::noteEditorHtmlUpdated, this,
        &NoteEditorWidget::onEditorHtmlUpdate);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::noteLoaded, this,
        &NoteEditorWidget::noteLoaded);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::noteModified, this,
        &NoteEditorWidget::onNoteEditorModified);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textBoldState, this,
        &NoteEditorWidget::onEditorTextBoldStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textItalicState, this,
        &NoteEditorWidget::onEditorTextItalicStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textUnderlineState, this,
        &NoteEditorWidget::onEditorTextUnderlineStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textUnderlineState, this,
        &NoteEditorWidget::onEditorTextUnderlineStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textStrikethroughState, this,
        &NoteEditorWidget::onEditorTextStrikethroughStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textAlignLeftState, this,
        &NoteEditorWidget::onEditorTextAlignLeftStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textAlignCenterState, this,
        &NoteEditorWidget::onEditorTextAlignCenterStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textAlignRightState, this,
        &NoteEditorWidget::onEditorTextAlignRightStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textAlignFullState, this,
        &NoteEditorWidget::onEditorTextAlignFullStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textInsideOrderedListState, this,
        &NoteEditorWidget::onEditorTextInsideOrderedListStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textInsideUnorderedListState, this,
        &NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textInsideTableState, this,
        &NoteEditorWidget::onEditorTextInsideTableStateChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textFontFamilyChanged, this,
        &NoteEditorWidget::onEditorTextFontFamilyChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::textFontSizeChanged, this,
        &NoteEditorWidget::onEditorTextFontSizeChanged);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::insertTableDialogRequested, this,
        &NoteEditorWidget::onEditorTextInsertTableDialogRequested);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::spellCheckerNotReady, this,
        &NoteEditorWidget::onEditorSpellCheckerNotReady);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::spellCheckerReady, this,
        &NoteEditorWidget::onEditorSpellCheckerReady);

    QObject::connect(
        m_pUi->noteEditor, &NoteEditor::notifyError, this,
        &NoteEditorWidget::notifyError);

    // Connect find and replace widget actions to local slots
    QObject::connect(
        m_pUi->findAndReplaceWidget, &FindAndReplaceWidget::closed, this,
        &NoteEditorWidget::onFindAndReplaceWidgetClosed);

    QObject::connect(
        m_pUi->findAndReplaceWidget, &FindAndReplaceWidget::textToFindEdited,
        this, &NoteEditorWidget::onTextToFindInsideNoteEdited);

    QObject::connect(
        m_pUi->findAndReplaceWidget, &FindAndReplaceWidget::findNext, this,
        &NoteEditorWidget::onFindNextInsideNote);

    QObject::connect(
        m_pUi->findAndReplaceWidget, &FindAndReplaceWidget::findPrevious, this,
        &NoteEditorWidget::onFindPreviousInsideNote);

    QObject::connect(
        m_pUi->findAndReplaceWidget,
        &FindAndReplaceWidget::searchCaseSensitivityChanged, this,
        &NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged);

    QObject::connect(
        m_pUi->findAndReplaceWidget, &FindAndReplaceWidget::replace, this,
        &NoteEditorWidget::onReplaceInsideNote);

    QObject::connect(
        m_pUi->findAndReplaceWidget, &FindAndReplaceWidget::replaceAll, this,
        &NoteEditorWidget::onReplaceAllInsideNote);

    // Connect toolbar buttons actions to local slots
    QObject::connect(
        m_pUi->fontBoldPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextBoldToggled);

    QObject::connect(
        m_pUi->fontItalicPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextItalicToggled);

    QObject::connect(
        m_pUi->fontUnderlinePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextUnderlineToggled);

    QObject::connect(
        m_pUi->fontStrikethroughPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextStrikethroughToggled);

    QObject::connect(
        m_pUi->formatJustifyLeftPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignLeftAction);

    QObject::connect(
        m_pUi->formatJustifyCenterPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignCenterAction);

    QObject::connect(
        m_pUi->formatJustifyRightPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignRightAction);

    QObject::connect(
        m_pUi->formatJustifyFullPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignFullAction);

    QObject::connect(
        m_pUi->insertHorizontalLinePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAddHorizontalLineAction);

    QObject::connect(
        m_pUi->formatIndentMorePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextIncreaseIndentationAction);

    QObject::connect(
        m_pUi->formatIndentLessPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextDecreaseIndentationAction);

    QObject::connect(
        m_pUi->formatListUnorderedPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextInsertUnorderedListAction);

    QObject::connect(
        m_pUi->formatListOrderedPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextInsertOrderedListAction);

    QObject::connect(
        m_pUi->formatAsSourceCodePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextFormatAsSourceCodeAction);

    QObject::connect(
        m_pUi->chooseTextColorToolButton, &ColorPickerToolButton::colorSelected,
        this, &NoteEditorWidget::onEditorChooseTextColor);

    QObject::connect(
        m_pUi->chooseBackgroundColorToolButton,
        &ColorPickerToolButton::colorSelected, this,
        &NoteEditorWidget::onEditorChooseBackgroundColor);

    QObject::connect(
        m_pUi->spellCheckBox, &QCheckBox::stateChanged, this,
        &NoteEditorWidget::onEditorSpellCheckStateChanged);

    QObject::connect(
        m_pUi->insertToDoCheckboxPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorInsertToDoCheckBoxAction);

    QObject::connect(
        m_pUi->insertTableToolButton, &InsertTableToolButton::createdTable,
        this, &NoteEditorWidget::onEditorInsertTable);

    QObject::connect(
        m_pUi->printNotePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onPrintNoteButtonPressed);

    QObject::connect(
        m_pUi->exportNoteToPdfPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onExportNoteToPdfButtonPressed);

    QObject::connect(
        m_pUi->exportNoteToEnexPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onExportNoteToEnexButtonPressed);

    // Connect toolbar button actions to editor slots
    QObject::connect(
        m_pUi->copyPushButton, &QPushButton::clicked, m_pUi->noteEditor,
        &NoteEditor::copy);

    QObject::connect(
        m_pUi->cutPushButton, &QPushButton::clicked, m_pUi->noteEditor,
        &NoteEditor::cut);

    QObject::connect(
        m_pUi->pastePushButton, &QPushButton::clicked, m_pUi->noteEditor,
        &NoteEditor::paste);

    QObject::connect(
        m_pUi->undoPushButton, &QPushButton::clicked, m_pUi->noteEditor,
        &NoteEditor::undo);

    QObject::connect(
        m_pUi->redoPushButton, &QPushButton::clicked, m_pUi->noteEditor,
        &NoteEditor::redo);

    QObject::connect(
        m_pUi->saveNotePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onSaveNoteAction);
}

void NoteEditorWidget::clear()
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::clear: note "
            << (m_pCurrentNote ? m_pCurrentNote->localId()
                               : QStringLiteral("<null>")));

    m_pCurrentNote.reset(nullptr);
    m_pCurrentNotebook.reset(nullptr);

    QObject::disconnect(
        m_pUi->noteEditor, &NoteEditor::noteAndNotebookFoundInLocalStorage,
        this, &NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage);

    QObject::disconnect(
        m_pUi->noteEditor, &NoteEditor::noteNotFound, this,
        &NoteEditorWidget::onNoteNotFoundInLocalStorage);

    m_pUi->noteEditor->clear();

    m_pUi->tagNameLabelsContainer->clear();
    m_pUi->noteNameLineEdit->clear();

    m_lastNoteTitleOrPreviewText.clear();

    m_findCurrentNotebookRequestId = QUuid();
    m_noteLinkInfoByFindNoteRequestIds.clear();

    m_pendingEditorSpellChecker = false;
    m_currentNoteWasExpunged = false;
    m_noteHasBeenModified = false;
}

void NoteEditorWidget::setupSpecialIcons()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::setupSpecialIcons");

    const QString enexIconName = QStringLiteral("application-enex");
    if (!QIcon::hasThemeIcon(enexIconName)) {
        QString fallbackIconThemeForEnexIcon = QStringLiteral("oxygen");
        const QString iconThemeName = QIcon::themeName();
        if (iconThemeName.contains(QStringLiteral("breeze")) ||
            (iconThemeName == QStringLiteral("tango")))
        {
            fallbackIconThemeForEnexIcon = iconThemeName;
        }

        QString fallbackIconExtension = QStringLiteral("png");
        if (fallbackIconThemeForEnexIcon.contains(QStringLiteral("breeze"))) {
            fallbackIconExtension = QStringLiteral("svg");
        }

        QNDEBUG(
            "widget:note_editor",
            "Fallback icon theme for enex icon = "
                << fallbackIconThemeForEnexIcon);

        // NOTE: it is necessary for the icon to have a name in order to ensure
        // the logic of icon switching works properly
        QIcon fallbackIcon = QIcon::fromTheme(enexIconName);

        fallbackIcon.addFile(
            QStringLiteral(":/icons/") + fallbackIconThemeForEnexIcon +
                QStringLiteral("/16x16/mimetypes/application-enex.") +
                fallbackIconExtension,
            QSize(16, 16), QIcon::Normal, QIcon::Off);

        fallbackIcon.addFile(
            QStringLiteral(":/icons/") + fallbackIconThemeForEnexIcon +
                QStringLiteral("/22x22/mimetypes/application-enex.") +
                fallbackIconExtension,
            QSize(22, 22), QIcon::Normal, QIcon::Off);

        fallbackIcon.addFile(
            QStringLiteral(":/icons/") + fallbackIconThemeForEnexIcon +
                QStringLiteral("/32x32/mimetypes/application-enex.") +
                fallbackIconExtension,
            QSize(32, 32), QIcon::Normal, QIcon::Off);

        m_pUi->exportNoteToEnexPushButton->setIcon(fallbackIcon);
    }
    else {
        QIcon themeIcon = QIcon::fromTheme(enexIconName);
        m_pUi->exportNoteToEnexPushButton->setIcon(themeIcon);
    }
}

void NoteEditorWidget::onNoteEditorColorsUpdate()
{
    setupNoteEditorColors();

    if (noteLocalId().isEmpty()) {
        m_pUi->noteEditor->clear();
        return;
    }

    if (!m_pCurrentNote || !m_pCurrentNotebook) {
        QNTRACE(
            "widget:note_editor",
            "Current note or notebook was not found "
                << "yet");
        return;
    }

    if (isModified()) {
        ErrorString errorDescription;
        auto status = checkAndSaveModifiedNote(errorDescription);
        if (status == NoteSaveStatus::Failed) {
            QNWARNING(
                "widget:note_editor",
                "Failed to save modified note: " << errorDescription);
            return;
        }

        if (status == NoteSaveStatus::Timeout) {
            QNWARNING(
                "widget:note_editor",
                "Failed to save modified note in "
                    << "due time");
            return;
        }
    }
}

void NoteEditorWidget::setupFontsComboBox()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::setupFontsComboBox");

    bool useLimitedFonts = useLimitedSetOfFonts();

    if (useLimitedFonts) {
        m_pUi->fontComboBox->setHidden(true);
        m_pUi->fontComboBox->setDisabled(true);
        setupLimitedFontsComboBox();
    }
    else {
        m_pUi->limitedFontComboBox->setHidden(true);
        m_pUi->limitedFontComboBox->setDisabled(true);

        QObject::connect(
            m_pUi->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);
    }

    setupNoteEditorDefaultFont();
}

void NoteEditorWidget::setupLimitedFontsComboBox(const QString & startupFont)
{
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::setupLimitedFontsComboBox: "
            << "startup font = " << startupFont);

    QStringList limitedFontNames;
    limitedFontNames.reserve(8);
    // NOTE: adding whitespace before the actual font names
    // simply because it looks better this way and I can't get
    // the stylesheet to handle this properly
    limitedFontNames << QStringLiteral(" Courier New");
    limitedFontNames << QStringLiteral(" Georgia");
    limitedFontNames << QStringLiteral(" Gotham");
    limitedFontNames << QStringLiteral(" Helvetica");
    limitedFontNames << QStringLiteral(" Trebuchet");
    limitedFontNames << QStringLiteral(" Times");
    limitedFontNames << QStringLiteral(" Times New Roman");
    limitedFontNames << QStringLiteral(" Verdana");

    delete m_pLimitedFontsListModel;
    m_pLimitedFontsListModel = new QStringListModel(this);
    m_pLimitedFontsListModel->setStringList(limitedFontNames);
    m_pUi->limitedFontComboBox->setModel(m_pLimitedFontsListModel);

    int currentIndex = limitedFontNames.indexOf(startupFont);
    if (currentIndex < 0) {
        currentIndex = 0;
    }

    m_pUi->limitedFontComboBox->setCurrentIndex(currentIndex);

    auto * pDelegate = qobject_cast<LimitedFontsDelegate *>(
        m_pUi->limitedFontComboBox->itemDelegate());

    if (!pDelegate) {
        pDelegate = new LimitedFontsDelegate(m_pUi->limitedFontComboBox);
        m_pUi->limitedFontComboBox->setItemDelegate(pDelegate);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->limitedFontComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);
#else
    QObject::connect(
        m_pUi->limitedFontComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onLimitedFontsComboBoxCurrentIndexChanged(int)));
#endif
}

void NoteEditorWidget::setupFontSizesComboBox()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::setupFontSizesComboBox");

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
    QNDEBUG(
        "widget:note_editor",
        "NoteEditorWidget::setupFontSizesForFont: "
            << "family = " << font.family()
            << ", style name = " << font.styleName());

    QFontDatabase fontDatabase;
    auto fontSizes = fontDatabase.pointSizes(font.family(), font.styleName());
    if (fontSizes.isEmpty()) {
        QNTRACE(
            "widget:note_editor",
            "Couldn't find point sizes for font "
                << "family " << font.family() << ", will use standard sizes "
                << "instead");

        fontSizes = QFontDatabase::standardSizes();
    }

    int currentFontSize = -1;
    int currentFontSizeIndex = m_pUi->fontSizeComboBox->currentIndex();

    QList<int> currentFontSizes;
    int currentCount = m_pUi->fontSizeComboBox->count();
    currentFontSizes.reserve(currentCount);
    for (int i = 0; i < currentCount; ++i) {
        bool conversionResult = false;
        QVariant data = m_pUi->fontSizeComboBox->itemData(i);
        int fontSize = data.toInt(&conversionResult);
        if (conversionResult) {
            currentFontSizes << fontSize;
            if (i == currentFontSizeIndex) {
                currentFontSize = fontSize;
            }
        }
    }

    if (currentFontSizes == fontSizes) {
        QNDEBUG(
            "widget:note_editor",
            "No need to update the items within font "
                << "sizes combo box: none of them have changed");
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::disconnect(
        m_pUi->fontSizeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged);
#else
    QObject::disconnect(
        m_pUi->fontSizeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onFontSizesComboBoxCurrentIndexChanged(int)));
#endif

    m_lastFontSizeComboBoxIndex = 0;
    m_pUi->fontSizeComboBox->clear();
    int numFontSizes = fontSizes.size();

    QNTRACE(
        "widget:note_editor",
        "Found " << numFontSizes << " font sizes for "
                 << "font family " << font.family());

    for (int i = 0; i < numFontSizes; ++i) {
        m_pUi->fontSizeComboBox->addItem(
            QString::number(fontSizes[i]), QVariant(fontSizes[i]));

        QNTRACE(
            "widget:note_editor",
            "Added item " << fontSizes[i] << "pt for index " << i);
    }

    m_lastFontSizeComboBoxIndex = -1;

    bool setFontSizeIndex = false;
    if (currentFontSize > 0) {
        int fontSizeIndex = fontSizes.indexOf(currentFontSize);
        if (fontSizeIndex >= 0) {
            m_pUi->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE(
                "widget:note_editor",
                "Setting the current font size to "
                    << "its previous value: " << currentFontSize);
            setFontSizeIndex = true;
        }
    }

    if (!setFontSizeIndex && (currentFontSize != 12)) {
        // Try to look for font size 12 as the sanest default font size
        int fontSizeIndex = fontSizes.indexOf(12);
        if (fontSizeIndex >= 0) {
            m_pUi->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE(
                "widget:note_editor",
                "Setting the current font size to "
                    << "the default value of 12");
            setFontSizeIndex = true;
        }
    }

    if (!setFontSizeIndex) {
        // Try to find any font size between 10 and 20, should be good enough
        for (int i = 0; i < numFontSizes; ++i) {
            int fontSize = fontSizes[i];
            if ((fontSize >= 10) && (fontSize <= 20)) {
                m_pUi->fontSizeComboBox->setCurrentIndex(i);
                QNTRACE(
                    "widget:note_editor",
                    "Setting the current font size "
                        << "to the default value of " << fontSize);
                setFontSizeIndex = true;
                break;
            }
        }
    }

    if (!setFontSizeIndex && !fontSizes.isEmpty()) {
        // All attempts to pick some sane font size have failed,
        // will just take the median (or only) font size
        int index = numFontSizes / 2;
        m_pUi->fontSizeComboBox->setCurrentIndex(index);
        QNTRACE(
            "widget:note_editor",
            "Setting the current font size to "
                << "the median value of " << fontSizes.at(index));
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->fontSizeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged,
        Qt::UniqueConnection);
#else
    QObject::connect(
        m_pUi->fontSizeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onFontSizesComboBoxCurrentIndexChanged(int)),
        Qt::UniqueConnection);
#endif
}

bool NoteEditorWidget::useLimitedSetOfFonts() const
{
    ApplicationSettings appSettings(
        m_currentAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    bool useLimitedFonts = false;
    if (appSettings.contains(preferences::keys::noteEditorUseLimitedSetOfFonts))
    {
        useLimitedFonts =
            appSettings.value(preferences::keys::noteEditorUseLimitedSetOfFonts)
                .toBool();

        QNDEBUG(
            "widget:note_editor",
            "Use limited fonts preference: "
                << (useLimitedFonts ? "true" : "false"));
    }
    else {
        useLimitedFonts = (m_currentAccount.type() == Account::Type::Evernote);
    }

    appSettings.endGroup();
    return useLimitedFonts;
}

void NoteEditorWidget::setupNoteEditorDefaultFont()
{
    QNDEBUG(
        "widget:note_editor", "NoteEditorWidget::setupNoteEditorDefaultFont");

    bool useLimitedFonts = !(m_pUi->limitedFontComboBox->isHidden());

    int pointSize = -1;
    int fontSizeIndex = m_pUi->fontSizeComboBox->currentIndex();
    if (fontSizeIndex >= 0) {
        bool conversionResult = false;
        QVariant fontSizeData =
            m_pUi->fontSizeComboBox->itemData(fontSizeIndex);
        pointSize = fontSizeData.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING(
                "widget:note_editor",
                "Failed to convert current font "
                    << "size to int: " << fontSizeData);
            pointSize = -1;
        }
    }

    QFont currentFont;
    if (useLimitedFonts) {
        QString fontFamily = m_pUi->limitedFontComboBox->currentText();
        currentFont = QFont(fontFamily, pointSize);
    }
    else {
        QFont font = m_pUi->fontComboBox->currentFont();
        currentFont = QFont(font.family(), pointSize);
    }

    m_pUi->noteEditor->setDefaultFont(currentFont);
}

void NoteEditorWidget::setupNoteEditorColors()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::setupNoteEditorColors");

    QPalette pal;

    ApplicationSettings appSettings(
        m_currentAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QString fontColorName =
        appSettings.value(preferences::keys::noteEditorFontColor).toString();

    QColor fontColor(fontColorName);
    if (fontColor.isValid()) {
        pal.setColor(QPalette::WindowText, fontColor);
    }

    QString backgroundColorName =
        appSettings.value(preferences::keys::noteEditorBackgroundColor)
            .toString();

    QColor backgroundColor(backgroundColorName);
    if (backgroundColor.isValid()) {
        pal.setColor(QPalette::Base, backgroundColor);
    }

    QString highlightColorName =
        appSettings.value(preferences::keys::noteEditorHighlightColor)
            .toString();

    QColor highlightColor(highlightColorName);
    if (highlightColor.isValid()) {
        pal.setColor(QPalette::Highlight, highlightColor);
    }

    QString highlightedTextColorName =
        appSettings.value(preferences::keys::noteEditorHighlightedTextColor)
            .toString();

    QColor highlightedTextColor(highlightedTextColorName);
    if (highlightedTextColor.isValid()) {
        pal.setColor(QPalette::HighlightedText, highlightedTextColor);
    }

    appSettings.endGroup();

    m_pUi->noteEditor->setDefaultPalette(pal);
}

void NoteEditorWidget::updateNoteSourceView(const QString & html)
{
    m_pUi->noteSourceView->setPlainText(html);
}

void NoteEditorWidget::setNoteAndNotebook(
    const qevercloud::Note & note, const qevercloud::Notebook & notebook)
{
    QNDEBUG(
        "widget:note_editor", "NoteEditorWidget::setCurrentNoteAndNotebook");

    QNTRACE(
        "widget:note_editor", "Note: " << note << "\nNotebook: " << notebook);

    if (!m_pCurrentNote) {
        m_pCurrentNote = std::make_unique<qevercloud::Note>(note);
    }
    else {
        *m_pCurrentNote = note;
    }

    if (!m_pCurrentNotebook) {
        m_pCurrentNotebook = std::make_unique<qevercloud::Notebook>(notebook);
    }
    else {
        *m_pCurrentNotebook = notebook;
    }

    m_pUi->noteNameLineEdit->show();
    m_pUi->tagNameLabelsContainer->show();

    if (!m_noteTitleIsEdited && note.title()) {
        QString title = *note.title();
        m_pUi->noteNameLineEdit->setText(title);
        if (m_lastNoteTitleOrPreviewText != title) {
            m_lastNoteTitleOrPreviewText = title;
            Q_EMIT titleOrPreviewChanged(m_lastNoteTitleOrPreviewText);
        }
    }
    else if (!m_noteTitleIsEdited) {
        m_pUi->noteNameLineEdit->clear();

        QString previewText;
        if (note.content()) {
            previewText = noteContentToPlainText(*note.content());
            previewText.truncate(140);
        }

        if (previewText != m_lastNoteTitleOrPreviewText) {
            m_lastNoteTitleOrPreviewText = previewText;
            Q_EMIT titleOrPreviewChanged(m_lastNoteTitleOrPreviewText);
        }
    }

    m_pUi->noteEditor->setCurrentNoteLocalId(note.localId());
    m_pUi->tagNameLabelsContainer->setCurrentNoteAndNotebook(note, notebook);

    m_pUi->printNotePushButton->setDisabled(false);
    m_pUi->exportNoteToPdfPushButton->setDisabled(false);
    m_pUi->exportNoteToEnexPushButton->setDisabled(false);
}

QString NoteEditorWidget::blankPageHtml() const
{
    QString html = QStringLiteral(
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" "
        "\"http://www.w3.org/TR/html4/strict.dtd\">"
        "<html><head>"
        "<meta http-equiv=\"Content-Type\" content=\"text/html\" "
        "charset=\"UTF-8\" />"
        "<style>"
        "body {"
        "background-color: ");

    QColor backgroundColor = palette().color(QPalette::Window).darker(115);
    html += backgroundColor.name();

    html += QStringLiteral(
        ";"
        "color: ");
    QColor foregroundColor = palette().color(QPalette::WindowText);
    html += foregroundColor.name();

    html += QStringLiteral(
        ";"
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
        "<body><div class=\"outer\"><div class=\"middle\">"
        "<div class=\"inner\">\n\n\n");

    html += tr("Please select some existing note or create a new one");

    html += QStringLiteral("</div></div></div></body></html>");

    return html;
}

void NoteEditorWidget::setupBlankEditor()
{
    QNDEBUG("widget:note_editor", "NoteEditorWidget::setupBlankEditor");

    m_pUi->printNotePushButton->setDisabled(true);
    m_pUi->exportNoteToPdfPushButton->setDisabled(true);
    m_pUi->exportNoteToEnexPushButton->setDisabled(true);

    m_pUi->noteNameLineEdit->hide();
    m_pUi->tagNameLabelsContainer->hide();

    QString initialHtml = blankPageHtml();
    m_pUi->noteEditor->setInitialPageHtml(initialHtml);

    m_pUi->findAndReplaceWidget->setHidden(true);
    m_pUi->noteSourceView->setHidden(true);
}

bool NoteEditorWidget::checkNoteTitle(
    const QString & title, ErrorString & errorDescription)
{
    if (title.isEmpty()) {
        return true;
    }

    return validateNoteTitle(title, &errorDescription);
}

void NoteEditorWidget::removeSurrondingApostrophes(QString & str) const
{
    if (str.startsWith(QStringLiteral("\'")) &&
        str.endsWith(QStringLiteral("\'")) && (str.size() > 1))
    {
        str = str.mid(1, str.size() - 2);
        QNTRACE("widget:note_editor", "Removed apostrophes: " << str);
    }
}

QTextStream & NoteEditorWidget::NoteLinkInfo::print(QTextStream & strm) const
{
    strm << "User id = " << m_userId << ", shard id = " << m_shardId
         << ", note guid = " << m_noteGuid;
    return strm;
}

QDebug & operator<<(QDebug & dbg, const NoteEditorWidget::NoteSaveStatus status)
{
    using NoteSaveStatus = NoteEditorWidget::NoteSaveStatus;

    switch (status) {
    case NoteSaveStatus::Ok:
        dbg << "Ok";
        break;
    case NoteSaveStatus::Failed:
        dbg << "Failed";
        break;
    case NoteSaveStatus::Timeout:
        dbg << "Timeout";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(status) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier

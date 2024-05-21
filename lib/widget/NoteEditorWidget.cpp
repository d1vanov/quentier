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

#include "NoteEditorWidget.h"

#include "FindAndReplaceWidget.h"
#include "NewListItemLineEdit.h"
#include "NoteTagsWidget.h"

#include "color-picker-tool-button/ColorPickerToolButton.h"
#include "insert-table-tool-button/InsertTableToolButton.h"
#include "insert-table-tool-button/TableSettingsDialog.h"

#include <lib/delegate/LimitedFontsDelegate.h>
#include <lib/enex/EnexExportDialog.h>
#include <lib/exception/Utils.h>
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

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/note_editor/SpellChecker.h>
#include <quentier/threading/Future.h>
#include <quentier/types/NoteUtils.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StandardPaths.h>

#include <QColor>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMimeData>
#include <QPalette>
#include <QPrintDialog>
#include <QStringListModel>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QToolTip>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace quentier {

NoteEditorWidget::NoteEditorWidget(
    Account account, local_storage::ILocalStoragePtr localStorage,
    SpellChecker & spellChecker, QThread * backgroundJobsThread,
    NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
    TagModel & tagModel, QUndoStack * undoStack, QWidget * parent) :
    QWidget{parent}, m_localStorage{std::move(localStorage)},
    m_ui{new Ui::NoteEditorWidget}, m_noteCache{noteCache},
    m_notebookCache{notebookCache}, m_tagCache{tagCache},
    m_currentAccount{std::move(account)}, m_undoStack{undoStack}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "NoteEditorWidget ctor: local storage is null"}};
    }

    m_ui->setupUi(this);
    setAcceptDrops(true);

    setupSpecialIcons();
    setupFontsComboBox();
    setupFontSizesComboBox();

    // Originally the NoteEditorWidget is not a window, so hide these two
    // buttons, they are for the separate-window mode only
    m_ui->printNotePushButton->setHidden(true);
    m_ui->exportNoteToPdfPushButton->setHidden(true);

    m_ui->noteEditor->initialize(
        m_localStorage, spellChecker, m_currentAccount, backgroundJobsThread);

    m_ui->saveNotePushButton->setEnabled(false);
    m_ui->noteNameLineEdit->installEventFilter(this);
    m_ui->noteEditor->setUndoStack(m_undoStack.data());

    setupNoteEditorColors();
    setupBlankEditor();

    m_ui->noteEditor->backend()->widget()->installEventFilter(this);

    auto * highlighter =
        new BasicXMLSyntaxHighlighter(m_ui->noteSourceView->document());

    Q_UNUSED(highlighter);

    m_ui->tagNameLabelsContainer->setTagModel(&tagModel);
    m_ui->tagNameLabelsContainer->setLocalStorage(*m_localStorage);

    createConnections();
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
        "widget::NoteEditorWidget",
        "NoteEditorWidget::setNoteLocalId: " << noteLocalId
                                             << ", is new note = "
                                             << (isNewNote ? "true" : "false"));

    if (m_noteLocalId == noteLocalId) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Note local id is already set to the editor, nothing to do");
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
        m_ui->noteEditor, &NoteEditor::noteAndNotebookFoundInLocalStorage,
        this, &NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::noteNotFound, this,
        &NoteEditorWidget::onNoteNotFoundInLocalStorage,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    m_ui->noteEditor->setCurrentNoteLocalId(noteLocalId);
}

bool NoteEditorWidget::isResolved() const noexcept
{
    return m_currentNote && m_currentNotebook;
}

bool NoteEditorWidget::isModified() const noexcept
{
    return m_currentNote &&
        (m_ui->noteEditor->isModified() || m_noteTitleIsEdited);
}

bool NoteEditorWidget::hasBeenModified() const noexcept
{
    return m_noteHasBeenModified || m_noteTitleHasBeenEdited;
}

qint64 NoteEditorWidget::idleTime() const noexcept
{
    if (!m_currentNote || !m_currentNotebook) {
        return -1;
    }

    return m_ui->noteEditor->idleTime();
}

QString NoteEditorWidget::titleOrPreview() const
{
    if (Q_UNLIKELY(!m_currentNote)) {
        return {};
    }

    if (m_currentNote->title()) {
        return *m_currentNote->title();
    }

    if (m_currentNote->content()) {
        QString previewText = noteContentToPlainText(*m_currentNote->content());
        previewText.truncate(140);
        return previewText;
    }

    return {};
}

const std::optional<qevercloud::Note> & NoteEditorWidget::currentNote()
    const noexcept
{
    return m_currentNote;
}

bool NoteEditorWidget::isNoteSourceShown() const
{
    return m_ui->noteSourceView->isVisible();
}

void NoteEditorWidget::showNoteSource()
{
    updateNoteSourceView(m_lastNoteEditorHtml);
    m_ui->noteSourceView->setHidden(false);
}

void NoteEditorWidget::hideNoteSource()
{
    m_ui->noteSourceView->setHidden(true);
}

bool NoteEditorWidget::isSpellCheckEnabled() const
{
    return m_ui->noteEditor->spellCheckEnabled();
}

NoteEditorWidget::NoteSaveStatus
NoteEditorWidget::checkAndSaveModifiedNote(ErrorString & errorDescription)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::checkAndSaveModifiedNote");

    if (!m_currentNote) {
        QNDEBUG("widget::NoteEditorWidget", "No note is set to the editor");
        return NoteSaveStatus::Ok;
    }

    if (m_currentNote->deleted()) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "The note is deleted which means it just got deleted and the "
            "editor is closing => there is no need to save whatever is left in "
            "the editor for this note");
        return NoteSaveStatus::Ok;
    }

    const bool noteContentModified = m_ui->noteEditor->isModified();
    if (!m_noteTitleIsEdited && !noteContentModified) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Note is not modified, nothing to save");
        return NoteSaveStatus::Ok;
    }

    bool noteTitleUpdated = false;

    if (m_noteTitleIsEdited) {
        QString noteTitle = m_ui->noteNameLineEdit->text().trimmed();
        m_stringUtils.removeNewlines(noteTitle);

        if (noteTitle.isEmpty() && m_currentNote->title()) {
            m_currentNote->setTitle(QString{});
            noteTitleUpdated = true;
        }
        else if (!noteTitle.isEmpty() && m_currentNote->title() != noteTitle) {
            ErrorString error;
            if (checkNoteTitle(noteTitle, error)) {
                m_currentNote->setTitle(noteTitle);
                noteTitleUpdated = true;
            }
            else {
                QNDEBUG(
                    "widget::NoteEditorWidget",
                    "Couldn't save the note title: " << error);
            }
        }
    }

    if (noteTitleUpdated) {
        // NOTE: indicating early that the note's title has been changed
        // manually and not generated automatically
        if (m_currentNote->attributes()) {
            m_currentNote->mutableAttributes()->setNoteTitleQuality(
                std::nullopt);
        }
    }

    if (noteContentModified || noteTitleUpdated) {
        ApplicationSettings appSettings;
        appSettings.beginGroup(preferences::keys::noteEditorGroup);

        const QVariant editorConvertToNoteTimeoutData = appSettings.value(
            preferences::keys::noteEditorConvertToNoteTimeout);

        appSettings.endGroup();

        bool conversionResult = false;

        int editorConvertToNoteTimeout =
            editorConvertToNoteTimeoutData.toInt(&conversionResult);

        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(
                "widget::NoteEditorWidget",
                "Can't read the timeout for note editor to note conversion "
                    << "from the application settings, fallback to the default "
                    << "value of "
                    << preferences::defaults::convertToNoteTimeout
                    << " milliseconds");

            editorConvertToNoteTimeout =
                preferences::defaults::convertToNoteTimeout;
        }
        else {
            editorConvertToNoteTimeout =
                std::max(editorConvertToNoteTimeout, 100);
        }

        if (m_convertToNoteDeadlineTimer) {
            m_convertToNoteDeadlineTimer->deleteLater();
        }

        m_convertToNoteDeadlineTimer = new QTimer{this};
        m_convertToNoteDeadlineTimer->setSingleShot(true);

        EventLoopWithExitStatus eventLoop;

        QObject::connect(
            m_convertToNoteDeadlineTimer, &QTimer::timeout, &eventLoop,
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

        m_convertToNoteDeadlineTimer->start(editorConvertToNoteTimeout);

        QTimer::singleShot(0, this, SLOT(updateNoteInLocalStorage()));

        Q_UNUSED(eventLoop.exec(QEventLoop::ExcludeUserInputEvents))
        auto status = eventLoop.exitStatus();

        if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
            errorDescription.setBase(
                QT_TR_NOOP("Failed to convert the editor contents to note"));
            QNWARNING("widget::NoteEditorWidget", errorDescription);
            return NoteSaveStatus::Failed;
        }

        if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
            errorDescription.setBase(
                QT_TR_NOOP("The conversion of note editor contents to note "
                           "failed to finish in time"));
            QNWARNING("widget::NoteEditorWidget", errorDescription);
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
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::makeSeparateWindow");

    if (isSeparateWindow()) {
        return false;
    }

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::Window;

    QWidget::setWindowFlags(flags);

    m_ui->printNotePushButton->setHidden(false);
    m_ui->exportNoteToPdfPushButton->setHidden(false);
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

    m_ui->printNotePushButton->setHidden(true);
    m_ui->exportNoteToPdfPushButton->setHidden(true);
    return true;
}

void NoteEditorWidget::setFocusToEditor()
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::setFocusToEditor");
    m_ui->noteEditor->setFocus();
}

bool NoteEditorWidget::printNote(ErrorString & errorDescription)
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::printNote");

    if (Q_UNLIKELY(!m_currentNote)) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't print note: no note is set to the editor"));
        QNDEBUG("widget::NoteEditorWidget", errorDescription);
        return false;
    }

    QPrinter printer;
    auto printDialog = std::make_unique<QPrintDialog>(&printer, this);
    printDialog->setWindowModality(Qt::WindowModal);

    QAbstractPrintDialog::PrintDialogOptions options;
    options |= QAbstractPrintDialog::PrintToFile;
    options |= QAbstractPrintDialog::PrintCollateCopies;

    printDialog->setOptions(options);
    if (printDialog->exec() == QDialog::Accepted) {
        return m_ui->noteEditor->print(printer, errorDescription);
    }

    QNTRACE("widget::NoteEditorWidget", "Note printing has been cancelled");
    return false;
}

bool NoteEditorWidget::exportNoteToPdf(ErrorString & errorDescription)
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::exportNoteToPdf");

    if (Q_UNLIKELY(!m_currentNote)) {
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

    auto fileDialog = std::make_unique<QFileDialog>(
        this, tr("Please select the output pdf file"), lastExportNoteToPdfPath);

    fileDialog->setWindowModality(Qt::WindowModal);
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->setDefaultSuffix(QStringLiteral("pdf"));

    QString suggestedFileName;
    if (m_currentNote->title()) {
        suggestedFileName = *m_currentNote->title();
    }
    else if (m_currentNote->content()) {
        suggestedFileName =
            noteContentToPlainText(*m_currentNote->content()).simplified();
        suggestedFileName.truncate(30);
    }

    if (!suggestedFileName.isEmpty()) {
        suggestedFileName += QStringLiteral(".pdf");
        fileDialog->selectFile(suggestedFileName);
    }

    if (fileDialog->exec() == QDialog::Accepted) {
        const QStringList selectedFiles = fileDialog->selectedFiles();
        const auto numSelectedFiles = selectedFiles.size();

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

        QFileInfo selectedFileInfo{selectedFiles[0]};
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
                QNDEBUG(
                    "widget::NoteEditorWidget", "Cancelled the export to pdf");
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

        return m_ui->noteEditor->exportToPdf(
            selectedFiles[0], errorDescription);
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "Exporting the note to pdf has been cancelled");

    return false;
}

bool NoteEditorWidget::exportNoteToEnex(ErrorString & errorDescription)
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::exportNoteToEnex");

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

    auto exportEnexDialog = std::make_unique<EnexExportDialog>(
        m_currentAccount, this, titleOrPreview());

    exportEnexDialog->setWindowModality(Qt::WindowModal);

    if (exportEnexDialog->exec() == QDialog::Accepted) {
        QNDEBUG("widget::NoteEditorWidget", "Confirmed ENEX export");

        const QString enexFilePath = exportEnexDialog->exportEnexFilePath();
        QFileInfo enexFileInfo{enexFilePath};
        if (enexFileInfo.exists()) {
            if (!enexFileInfo.isWritable()) {
                errorDescription.setBase(
                    QT_TR_NOOP("Can't export note to ENEX: the selected file "
                               "already exists and is not writable"));
                QNWARNING("widget::NoteEditorWidget", errorDescription);
                return false;
            }

            QNDEBUG(
                "widget::NoteEditorWidget",
                "The file selected for ENEX export already exists");

            const int res = questionMessageBox(
                this, tr("Enex file already exists"),
                tr("The file selected for ENEX export already exists"),
                tr("Do you wish to overwrite the existing file?"));

            if (res != QMessageBox::Ok) {
                QNDEBUG(
                    "widget::NoteEditorWidget",
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
                    QNWARNING("widget::NoteEditorWidget", errorDescription);
                    return false;
                }
            }
        }

        const QStringList tagNames =
            (exportEnexDialog->exportTags()
                 ? m_ui->tagNameLabelsContainer->tagNames()
                 : QStringList());

        QString enex;
        if (!m_ui->noteEditor->exportToEnex(tagNames, enex, errorDescription)) {
            QNDEBUG(
                "widget::NoteEditorWidget",
                "ENEX export failed: " << errorDescription);
            return false;
        }

        QFile enexFile{enexFilePath};
        if (!enexFile.open(QIODevice::WriteOnly)) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't export note to ENEX: can't open the target "
                           "ENEX file for writing"));
            QNWARNING("widget::NoteEditorWidget", errorDescription);
            return false;
        }

        QByteArray rawEnexData = enex.toUtf8();
        const qint64 bytes = enexFile.write(rawEnexData);
        enexFile.close();

        if (Q_UNLIKELY(bytes != rawEnexData.size())) {
            errorDescription.setBase(
                QT_TR_NOOP("Writing the ENEX to a file was not completed "
                           "successfully"));

            errorDescription.details() = QStringLiteral("Bytes written = ") +
                QString::number(bytes) + QStringLiteral(" while expected ") +
                QString::number(rawEnexData.size());

            QNWARNING("widget::NoteEditorWidget", errorDescription);
            return false;
        }

        QNDEBUG(
            "widget::NoteEditorWidget",
            "Successfully exported the note to ENEX");
        return true;
    }

    QNTRACE(
        "widget::NoteEditorWidget",
        "Exporting the note to ENEX has been cancelled");

    return false;
}

void NoteEditorWidget::refreshSpecialIcons()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::refreshSpecialIcons, "
            << "note local id = " << m_noteLocalId);

    setupSpecialIcons();
}

void NoteEditorWidget::dragEnterEvent(QDragEnterEvent * event)
{
    QNTRACE("widget::NoteEditorWidget", "NoteEditorWidget::dragEnterEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "Detected null pointer to QDragEnterEvent in NoteEditorWidget's "
            "dragEnterEvent");
        return;
    }

    const auto * mimeData = event->mimeData();
    if (Q_UNLIKELY(!mimeData)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "Null pointer to mime data from drag enter event was detected");
        return;
    }

    if (!mimeData->hasFormat(TagModel::mimeTypeName())) {
        QNTRACE("widget::NoteEditorWidget", "Not tag mime type, skipping");
        return;
    }

    event->acceptProposedAction();
}

void NoteEditorWidget::dragMoveEvent(QDragMoveEvent * event)
{
    QNTRACE("widget::NoteEditorWidget", "NoteEditorWidget::dragMoveEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "Detected null pointer to QDropMoveEvent in NoteEditorWidget's "
            "dragMoveEvent");
        return;
    }

    const auto * mimeData = event->mimeData();
    if (Q_UNLIKELY(!mimeData)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "Null pointer to mime data from drag move event was detected");
        return;
    }

    if (!mimeData->hasFormat(TagModel::mimeTypeName())) {
        QNTRACE("widget::NoteEditorWidget", "Not tag mime type, skipping");
        return;
    }

    event->acceptProposedAction();
}

void NoteEditorWidget::dropEvent(QDropEvent * event)
{
    QNTRACE("widget::NoteEditorWidget", "NoteEditorWidget::dropEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "Detected null pointer to QDropEvent in NoteEditorWidget's "
            "dropEvent");
        return;
    }

    const auto * mimeData = event->mimeData();
    if (Q_UNLIKELY(!mimeData)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "Null pointer to mime data from drop event was detected");
        return;
    }

    const QString mimeTypeName = TagModel::mimeTypeName();
    if (!mimeData->hasFormat(mimeTypeName)) {
        return;
    }

    QByteArray data = qUncompress(mimeData->data(mimeTypeName));
    QDataStream in{&data, QIODevice::ReadOnly};

    qint32 type = 0;
    in >> type;

    if (type != static_cast<qint32>(ITagModelItem::Type::Tag)) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Can only drop tag model items of tag type onto NoteEditorWidget");
        return;
    }

    TagItem item;
    in >> item;

    if (Q_UNLIKELY(!m_currentNote)) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Can't drop tag onto NoteEditorWidget: no note is set to the "
            "editor");
        return;
    }

    if (m_currentNote->tagLocalIds().contains(item.localId())) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Note set to the note editor ("
                << m_currentNote->localId()
                << ") is already marked with tag with local id "
                << item.localId());
        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "Adding tag with local id " << item.localId()
                                    << " to note with local id "
                                    << m_currentNote->localId());

    m_currentNote->mutableTagLocalIds().append(item.localId());

    if (!item.guid().isEmpty()) {
        if (!m_currentNote->tagGuids()) {
            m_currentNote->setTagGuids(QList<qevercloud::Guid>{item.guid()});
        }
        else if (!m_currentNote->tagGuids()->contains(item.guid())) {
            m_currentNote->mutableTagGuids()->append(item.guid());
        }
    }

    QStringList tagLocalIds = m_currentNote->tagLocalIds();
    QStringList tagGuids =
        m_currentNote->tagGuids().value_or(QList<qevercloud::Guid>{});

    m_ui->noteEditor->setTagIds(tagLocalIds, tagGuids);
    updateNoteInLocalStorage();

    event->acceptProposedAction();
}

void NoteEditorWidget::closeEvent(QCloseEvent * event)
{
    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "Detected null pointer to QCloseEvent in NoteEditorWidget's "
            "closeEvent");
        return;
    }

    ErrorString errorDescription;
    const auto status = checkAndSaveModifiedNote(errorDescription);

    QNDEBUG(
        "widget::NoteEditorWidget",
        "Check and save modified note, status: "
            << status << ", error description: " << errorDescription);

    event->accept();
}

bool NoteEditorWidget::eventFilter(QObject * watched, QEvent * event)
{
    if (Q_UNLIKELY(!watched)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "NoteEditorWidget: caught event for null watched object in the "
            "eventFilter method");
        return true;
    }

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "widget::NoteEditorWidget",
            "NoteEditorWidget: caught null event in the eventFilter method for "
                << "object " << watched->objectName() << " of class "
                << watched->metaObject()->className());
        return true;
    }

    const auto eventType = event->type();

    if (watched == m_ui->noteNameLineEdit) {
        if (eventType == QEvent::FocusIn) {
            QNDEBUG("widget::NoteEditorWidget", "Note title editor gained focus");
        }
        else if (eventType == QEvent::FocusOut) {
            QNDEBUG("widget::NoteEditorWidget", "Note title editor lost focus");
        }
    }
    else if (watched == m_ui->noteEditor) {
        if (eventType == QEvent::FocusIn) {
            QNDEBUG("widget::NoteEditorWidget", "Note editor gained focus");
        }
        else if (eventType == QEvent::FocusOut) {
            QNDEBUG("widget::NoteEditorWidget", "Note editor lost focus");
        }
    }
    else if (watched == m_ui->noteEditor->backend()->widget()) {
        if (eventType == QEvent::DragEnter) {
            QNTRACE(
                "widget::NoteEditorWidget",
                "Detected drag enter event for note editor");

            auto * eventDragEnter = dynamic_cast<QDragEnterEvent *>(event);
            if (eventDragEnter && eventDragEnter->mimeData() &&
                eventDragEnter->mimeData()->hasFormat(TagModel::mimeTypeName()))
            {
                QNDEBUG(
                    "widget::NoteEditorWidget",
                    "Stealing drag enter event with tag data from note editor");
                dragEnterEvent(eventDragEnter);
                return true;
            }
            else {
                QNTRACE(
                    "widget::NoteEditorWidget",
                    "Detected no tag data in the event");
            }
        }
        else if (eventType == QEvent::DragMove) {
            QNTRACE(
                "widget::NoteEditorWidget",
                "Detected drag move event for note editor");

            auto * eventDragMove = dynamic_cast<QDragMoveEvent *>(event);
            if (eventDragMove && eventDragMove->mimeData() &&
                eventDragMove->mimeData()->hasFormat(TagModel::mimeTypeName()))
            {
                QNDEBUG(
                    "widget::NoteEditorWidget",
                    "Stealing drag move event with tag data from note editor");
                dragMoveEvent(eventDragMove);
                return true;
            }
            else {
                QNTRACE(
                    "widget::NoteEditorWidget",
                    "Detected no tag data in the event");
            }
        }
        else if (eventType == QEvent::Drop) {
            QNTRACE(
                "widget::NoteEditorWidget",
                "Detected drop event for note editor");

            auto * eventDrop = dynamic_cast<QDropEvent *>(event);
            if (eventDrop && eventDrop->mimeData() &&
                eventDrop->mimeData()->hasFormat(TagModel::mimeTypeName()))
            {
                QNDEBUG(
                    "widget::NoteEditorWidget",
                    "Stealing drop event with tag data from note editor");
                dropEvent(eventDrop);
                return true;
            }
            else {
                QNTRACE(
                    "widget::NoteEditorWidget",
                    "Detected no tag data in the event");
            }
        }
    }

    return false;
}

void NoteEditorWidget::onEditorTextBoldToggled()
{
    m_ui->noteEditor->textBold();
}

void NoteEditorWidget::onEditorTextItalicToggled()
{
    m_ui->noteEditor->textItalic();
}

void NoteEditorWidget::onEditorTextUnderlineToggled()
{
    m_ui->noteEditor->textUnderline();
}

void NoteEditorWidget::onEditorTextStrikethroughToggled()
{
    m_ui->noteEditor->textStrikethrough();
}

void NoteEditorWidget::onEditorTextAlignLeftAction()
{
    if (m_ui->formatJustifyLeftPushButton->isChecked()) {
        m_ui->formatJustifyCenterPushButton->setChecked(false);
        m_ui->formatJustifyRightPushButton->setChecked(false);
        m_ui->formatJustifyFullPushButton->setChecked(false);
    }

    m_ui->noteEditor->alignLeft();
}

void NoteEditorWidget::onEditorTextAlignCenterAction()
{
    if (m_ui->formatJustifyCenterPushButton->isChecked()) {
        m_ui->formatJustifyLeftPushButton->setChecked(false);
        m_ui->formatJustifyRightPushButton->setChecked(false);
        m_ui->formatJustifyFullPushButton->setChecked(false);
    }

    m_ui->noteEditor->alignCenter();
}

void NoteEditorWidget::onEditorTextAlignRightAction()
{
    if (m_ui->formatJustifyRightPushButton->isChecked()) {
        m_ui->formatJustifyLeftPushButton->setChecked(false);
        m_ui->formatJustifyCenterPushButton->setChecked(false);
        m_ui->formatJustifyFullPushButton->setChecked(false);
    }

    m_ui->noteEditor->alignRight();
}

void NoteEditorWidget::onEditorTextAlignFullAction()
{
    if (m_ui->formatJustifyFullPushButton->isChecked()) {
        m_ui->formatJustifyLeftPushButton->setChecked(false);
        m_ui->formatJustifyCenterPushButton->setChecked(false);
        m_ui->formatJustifyRightPushButton->setChecked(false);
    }

    m_ui->noteEditor->alignFull();
}

void NoteEditorWidget::onEditorTextAddHorizontalLineAction()
{
    m_ui->noteEditor->insertHorizontalLine();
}

void NoteEditorWidget::onEditorTextIncreaseFontSizeAction()
{
    m_ui->noteEditor->increaseFontSize();
}

void NoteEditorWidget::onEditorTextDecreaseFontSizeAction()
{
    m_ui->noteEditor->decreaseFontSize();
}

void NoteEditorWidget::onEditorTextHighlightAction()
{
    m_ui->noteEditor->textHighlight();
}

void NoteEditorWidget::onEditorTextIncreaseIndentationAction()
{
    m_ui->noteEditor->increaseIndentation();
}

void NoteEditorWidget::onEditorTextDecreaseIndentationAction()
{
    m_ui->noteEditor->decreaseIndentation();
}

void NoteEditorWidget::onEditorTextInsertUnorderedListAction()
{
    m_ui->noteEditor->insertBulletedList();
}

void NoteEditorWidget::onEditorTextInsertOrderedListAction()
{
    m_ui->noteEditor->insertNumberedList();
}

void NoteEditorWidget::onEditorTextInsertToDoAction()
{
    m_ui->noteEditor->insertToDoCheckbox();
}

void NoteEditorWidget::onEditorTextFormatAsSourceCodeAction()
{
    m_ui->noteEditor->formatSelectionAsSourceCode();
}

void NoteEditorWidget::onEditorTextEditHyperlinkAction()
{
    m_ui->noteEditor->editHyperlinkDialog();
}

void NoteEditorWidget::onEditorTextCopyHyperlinkAction()
{
    m_ui->noteEditor->copyHyperlink();
}

void NoteEditorWidget::onEditorTextRemoveHyperlinkAction()
{
    m_ui->noteEditor->removeHyperlink();
}

void NoteEditorWidget::onEditorChooseTextColor(QColor color)
{
    m_ui->noteEditor->setFontColor(color);
}

void NoteEditorWidget::onEditorChooseBackgroundColor(QColor color)
{
    m_ui->noteEditor->setBackgroundColor(color);
}

void NoteEditorWidget::onEditorSpellCheckStateChanged(int state)
{
    m_ui->noteEditor->setSpellcheck(state != Qt::Unchecked);
}

void NoteEditorWidget::onEditorInsertToDoCheckBoxAction()
{
    m_ui->noteEditor->insertToDoCheckbox();
}

void NoteEditorWidget::onEditorInsertTableDialogAction()
{
    onEditorTextInsertTableDialogRequested();
}

void NoteEditorWidget::onEditorInsertTable(
    int rows, int columns, double width, const bool relativeWidth)
{
    rows = std::max(rows, 1);
    columns = std::max(columns, 1);
    width = std::max(width, 1.0);

    if (relativeWidth) {
        m_ui->noteEditor->insertRelativeWidthTable(rows, columns, width);
    }
    else {
        m_ui->noteEditor->insertFixedWidthTable(
            rows, columns, static_cast<int>(width));
    }

    QNTRACE(
        "widget::NoteEditorWidget",
        "Inserted table: rows = "
            << rows << ", columns = " << columns << ", width = " << width
            << ", relative width = " << (relativeWidth ? "true" : "false"));
}

void NoteEditorWidget::onUndoAction()
{
    m_ui->noteEditor->undo();
}

void NoteEditorWidget::onRedoAction()
{
    m_ui->noteEditor->redo();
}

void NoteEditorWidget::onCopyAction()
{
    m_ui->noteEditor->copy();
}

void NoteEditorWidget::onCutAction()
{
    m_ui->noteEditor->cut();
}

void NoteEditorWidget::onPasteAction()
{
    m_ui->noteEditor->paste();
}

void NoteEditorWidget::onSelectAllAction()
{
    m_ui->noteEditor->selectAll();
}

void NoteEditorWidget::onSaveNoteAction()
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::onSaveNoteAction");

    if (!m_ui->noteEditor->isModified()) {
        QNDEBUG("widget::NoteEditorWidget", "Note is not modified");
        m_ui->saveNotePushButton->setEnabled(false);
        return;
    }

    m_ui->saveNotePushButton->setEnabled(false);
    m_ui->noteEditor->saveNoteToLocalStorage();
}

void NoteEditorWidget::onSetUseLimitedFonts(const bool useLimitedFonts)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onSetUseLimitedFonts: "
            << (useLimitedFonts ? "true" : "false"));

    const bool currentlyUsingLimitedFonts =
        !(m_ui->limitedFontComboBox->isHidden());

    if (currentlyUsingLimitedFonts == useLimitedFonts) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "The setting has not changed, nothing to do");
        return;
    }

    const QString currentFontFamily =
        (useLimitedFonts ? m_ui->limitedFontComboBox->currentText()
                         : m_ui->fontComboBox->currentFont().family());

    if (useLimitedFonts) {
        QObject::disconnect(
            m_ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);

        m_ui->fontComboBox->setHidden(true);
        m_ui->fontComboBox->setDisabled(true);

        setupLimitedFontsComboBox(currentFontFamily);
        m_ui->limitedFontComboBox->setHidden(false);
        m_ui->limitedFontComboBox->setDisabled(false);
    }
    else {
        QObject::disconnect(
            m_ui->limitedFontComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);

        m_ui->limitedFontComboBox->setHidden(true);
        m_ui->limitedFontComboBox->setDisabled(true);

        QFont font;
        font.setFamily(currentFontFamily);
        m_ui->fontComboBox->setFont(font);

        m_ui->fontComboBox->setHidden(false);
        m_ui->fontComboBox->setDisabled(false);

        QObject::connect(
            m_ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);
    }
}

void NoteEditorWidget::onNoteTagsListChanged(qevercloud::Note note)
{
    QNDEBUG(
        "widget::NoteEditorWidget", "NoteEditorWidget::onNoteTagsListChanged");

    const auto tagLocalIds = note.tagLocalIds();
    m_ui->noteEditor->setTagIds(tagLocalIds, QStringList{});

    if (m_ui->noteEditor->isModified()) {
        m_ui->noteEditor->saveNoteToLocalStorage();
    }
}

void NoteEditorWidget::onNoteEditorFontColorChanged(const QColor & color)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteEditorFontColorChanged: " << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorBackgroundColorChanged(const QColor & color)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteEditorBackgroundColorChanged: "
            << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorHighlightColorChanged(const QColor & color)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteEditorHighlightColorChanged: "
            << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorHighlightedTextColorChanged(
    const QColor & color)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteEditorHighlightedTextColorChanged: "
            << color.name());

    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNoteEditorColorsReset()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteEditorColorsReset");
    onNoteEditorColorsUpdate();
}

void NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem");

    auto * focusWidget = qApp->focusWidget();
    if (focusWidget) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Clearing the focus from the widget having it currently: "
                << focusWidget);
        focusWidget->clearFocus();
    }

    setFocusToEditor();
}

void NoteEditorWidget::onFontComboBoxFontChanged(const QFont & font)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFontComboBoxFontChanged: font family = "
            << font.family());

    if (m_lastFontComboBoxFontFamily == font.family()) {
        QNTRACE("widget::NoteEditorWidget", "Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = font.family();

    if (!m_currentNote) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "No note is set to the editor, nothing to do");
        return;
    }

    m_ui->noteEditor->setFont(font);
}

void NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged(
    int fontFamilyIndex)
{
    QString fontFamily = m_ui->limitedFontComboBox->itemText(fontFamilyIndex);

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged: "
            << fontFamily);

    if (fontFamily.trimmed().isEmpty()) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Font family is empty, ignoring this update");
        return;
    }

    removeSurrondingApostrophes(fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE("widget::NoteEditorWidget", "Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    if (!m_currentNote) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "No note is set to the editor, nothing to do");
        return;
    }

    QFont font;
    font.setFamily(fontFamily);
    m_ui->noteEditor->setFont(font);
}

void NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged(const int index)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged: index = "
            << index);

    if (index == m_lastFontSizeComboBoxIndex) {
        QNTRACE("widget::NoteEditorWidget", "Font size hasn't changed");
        return;
    }

    m_lastFontSizeComboBoxIndex = index;

    if (!m_currentNote) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "No note is set to the editor, nothing to do");
        return;
    }

    if (m_lastFontSizeComboBoxIndex < 0) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Font size combo box index is negative, won't do anything");
        return;
    }

    bool conversionResult = false;
    const QVariant data = m_ui->fontSizeComboBox->itemData(index);
    const int fontSize = data.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString error{
            QT_TR_NOOP("Can't process the change of font size combo box index: "
                       "can't convert combo box item data to int")};
        QNWARNING("widget::NoteEditorWidget", error << ": " << data);
        Q_EMIT notifyError(error);
        return;
    }

    m_ui->noteEditor->setFontHeight(fontSize);
}

void NoteEditorWidget::onNoteSavedToLocalStorage(QString noteLocalId)
{
    if (Q_UNLIKELY(!m_currentNote || (m_currentNote->localId() != noteLocalId)))
    {
        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteSavedToLocalStorage: note local id = "
            << noteLocalId);

    Q_EMIT noteSavedInLocalStorage();
}

void NoteEditorWidget::onFailedToSaveNoteToLocalStorage(
    ErrorString errorDescription, QString noteLocalId)
{
    if (Q_UNLIKELY(!m_currentNote || (m_currentNote->localId() != noteLocalId)))
    {
        return;
    }

    QNWARNING(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFailedToSaveNoteToLocalStorage: "
            << "note local id = " << errorDescription
            << ", error description: " << errorDescription);

    m_ui->saveNotePushButton->setEnabled(true);

    ErrorString error{QT_TR_NOOP("Failed to save the updated note")};
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);

    // NOTE: not clearing out the unsaved stuff because it may be of value
    // to the user

    Q_EMIT noteSaveInLocalStorageFailed();
}

void NoteEditorWidget::onNoteTitleEdited(const QString & noteTitle)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteTitleEdited: " << noteTitle);

    m_noteTitleIsEdited = true;
    m_noteTitleHasBeenEdited = true;
}

void NoteEditorWidget::onNoteEditorModified()
{
    QNTRACE(
        "widget::NoteEditorWidget", "NoteEditorWidget::onNoteEditorModified");

    m_noteHasBeenModified = true;

    if (!m_ui->noteEditor->isNoteLoaded()) {
        QNTRACE("widget::NoteEditorWidget", "The note is still being loaded");
        return;
    }

    if (Q_LIKELY(m_ui->noteEditor->isModified())) {
        m_ui->saveNotePushButton->setEnabled(true);
    }
}

void NoteEditorWidget::onNoteTitleUpdated()
{
    QString noteTitle = m_ui->noteNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(noteTitle);

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteTitleUpdated: " << noteTitle);

    m_noteTitleIsEdited = false;
    m_noteTitleHasBeenEdited = true;

    if (Q_UNLIKELY(!m_currentNote)) {
        // That shouldn't really happen in normal circumstances but it could
        // in theory be some old event which has reached this object after
        // the note has already been cleaned up
        QNDEBUG(
            "widget::NoteEditorWidget",
            "No current note in the note editor widget! Ignoring the note "
            "title update");
        return;
    }

    if (!m_currentNote->title() && noteTitle.isEmpty()) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Note's title is still empty, nothing to do");
        return;
    }

    if (m_currentNote->title() && (m_currentNote->title() == noteTitle)) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Note's title hasn't changed, nothing to do");
        return;
    }

    ErrorString error;
    if (!checkNoteTitle(noteTitle, error)) {
        QPoint targetPoint{0, m_ui->noteNameLineEdit->height()};
        targetPoint = m_ui->noteNameLineEdit->mapToGlobal(targetPoint);
        QToolTip::showText(targetPoint, error.localizedString());
        return;
    }

    m_currentNote->setTitle(noteTitle);
    m_isNewNote = false;

    m_ui->noteEditor->setNoteTitle(noteTitle);

    Q_EMIT titleOrPreviewChanged(titleOrPreview());

    updateNoteInLocalStorage();
}

void NoteEditorWidget::onEditorNoteUpdate(qevercloud::Note note)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorNoteUpdate: note local id = "
            << note.localId());

    QNTRACE("widget::NoteEditorWidget", "Note: " << note);

    if (Q_UNLIKELY(!m_currentNote)) {
        // That shouldn't really happen in normal circumstances but it could
        // in theory be some old event which has reached this object after
        // the note has already been cleaned up
        QNDEBUG(
            "widget::NoteEditorWidget",
            "No current note in the note editor widget! Ignoring the update "
            "from note editor");
        return;
    }

    QString noteTitle = m_currentNote->title().value_or(QString{});
    *m_currentNote = note;
    m_currentNote->setTitle(std::move(noteTitle));

    updateNoteInLocalStorage();
}

void NoteEditorWidget::onEditorNoteUpdateFailed(const ErrorString & error)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorNoteUpdateFailed: " << error);

    Q_EMIT notifyError(error);
    Q_EMIT conversionToNoteFailed();
}

void NoteEditorWidget::onEditorInAppLinkPasteRequested(
    QString url, QString userId, QString shardId, QString noteGuid)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorInAppLinkPasteRequested: url = "
            << url << ", user id = " << userId << ", shard id = " << shardId
            << ", note guid = " << noteGuid);

    NoteLinkInfo noteLinkInfo;
    noteLinkInfo.m_userId = userId;
    noteLinkInfo.m_shardId = shardId;
    noteLinkInfo.m_noteGuid = noteGuid;

    QNDEBUG(
        "widget::NoteEditorWidget",
        "Trying to find note by guid in the local storage "
            << "for the purpose of in-app note link insertion: note guid = "
            << noteGuid);

    auto findNoteFuture = m_localStorage->findNoteByGuid(
        noteGuid, local_storage::ILocalStorage::FetchNoteOptions{});

    auto findNoteThenFuture = threading::then(
        std::move(findNoteFuture), this,
        [this, noteLinkInfo](const std::optional<qevercloud::Note> & note) {
            if (!note) {
                QNDEBUG(
                    "widget::NoteEditorWidget",
                    "Inserting in-app note link without link text as the "
                        << "referenced note was not found: user id = "
                        << noteLinkInfo.m_userId
                        << ", shard id = " << noteLinkInfo.m_shardId
                        << ", note guid = " << noteLinkInfo.m_noteGuid);

                Q_EMIT insertInAppNoteLink(
                    noteLinkInfo.m_userId, noteLinkInfo.m_shardId,
                    noteLinkInfo.m_noteGuid, QString{});
                return;
            }

            QString titleOrPreview;
            if (note->title()) {
                titleOrPreview = *note->title();
            }
            else if (note->content()) {
                titleOrPreview = noteContentToPlainText(*note->content());
                titleOrPreview.truncate(140);
            }

            QNDEBUG(
                "widget::NoteEditorWidget",
                "Inserting in-app note link: user id = "
                    << noteLinkInfo.m_userId << ", shard id = "
                    << noteLinkInfo.m_shardId << ", note guid = "
                    << noteLinkInfo.m_noteGuid << ", title or preview = "
                    << titleOrPreview);

            Q_EMIT insertInAppNoteLink(
                noteLinkInfo.m_userId, noteLinkInfo.m_shardId,
                noteLinkInfo.m_noteGuid, titleOrPreview);
        });

    threading::onFailed(
        std::move(findNoteThenFuture), this,
        [this, noteLinkInfo = std::move(noteLinkInfo)](const QException & e) {
            const auto message = exceptionMessage(e);
            QNWARNING(
                "widget::NoteEditorWidget",
                "Failed to find note by guid in the local storage: " << message
                    << ", insering in-app note link without link text: "
                    << "user id = "
                    << noteLinkInfo.m_userId
                    << ", shard id = " << noteLinkInfo.m_shardId
                    << ", note guid = " << noteLinkInfo.m_noteGuid);

            Q_EMIT insertInAppNoteLink(
                noteLinkInfo.m_userId, noteLinkInfo.m_shardId,
                noteLinkInfo.m_noteGuid, QString{});
        });
}

void NoteEditorWidget::onEditorTextBoldStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextBoldStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->fontBoldPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextItalicStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextItalicStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->fontItalicPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextUnderlineStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextUnderlineStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->fontUnderlinePushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextStrikethroughStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextStrikethroughStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->fontStrikethroughPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextAlignLeftStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextAlignLeftStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->formatJustifyLeftPushButton->setChecked(state);

    if (state) {
        m_ui->formatJustifyCenterPushButton->setChecked(false);
        m_ui->formatJustifyRightPushButton->setChecked(false);
        m_ui->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignCenterStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextAlignCenterStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->formatJustifyCenterPushButton->setChecked(state);

    if (state) {
        m_ui->formatJustifyLeftPushButton->setChecked(false);
        m_ui->formatJustifyRightPushButton->setChecked(false);
        m_ui->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignRightStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextAlignRightStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->formatJustifyRightPushButton->setChecked(state);

    if (state) {
        m_ui->formatJustifyLeftPushButton->setChecked(false);
        m_ui->formatJustifyCenterPushButton->setChecked(false);
        m_ui->formatJustifyFullPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignFullStateChanged(const bool state)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextAlignFullStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->formatJustifyFullPushButton->setChecked(state);

    if (state) {
        m_ui->formatJustifyLeftPushButton->setChecked(false);
        m_ui->formatJustifyCenterPushButton->setChecked(false);
        m_ui->formatJustifyRightPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideOrderedListStateChanged(
    const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextInsideOrderedListStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->formatListOrderedPushButton->setChecked(state);

    if (state) {
        m_ui->formatListUnorderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged(
    const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->formatListUnorderedPushButton->setChecked(state);

    if (state) {
        m_ui->formatListOrderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideTableStateChanged(const bool state)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextInsideTableStateChanged: "
            << (state ? "enabled" : "disabled"));

    m_ui->insertTableToolButton->setEnabled(!state);
}

void NoteEditorWidget::onEditorTextFontFamilyChanged(QString fontFamily)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextFontFamilyChanged: " << fontFamily);

    removeSurrondingApostrophes(fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE("widget::NoteEditorWidget", "Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    QFont currentFont{fontFamily};

    if (!m_ui->limitedFontComboBox->isHidden()) {
        if (Q_UNLIKELY(!m_limitedFontsListModel)) {
            ErrorString errorDescription{
                QT_TR_NOOP("Can't process the change of font: internal error, "
                           "no limited fonts list model is set up")};

            QNWARNING("widget::NoteEditorWidget", errorDescription);
            Q_EMIT notifyError(errorDescription);
            return;
        }

        QStringList fontNames = m_limitedFontsListModel->stringList();
        auto index = fontNames.indexOf(fontFamily);
        if (index < 0) {
            QNDEBUG(
                "widget::NoteEditorWidget",
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
                        "widget::NoteEditorWidget",
                        "Note editor's font family "
                            << fontFamily << " appears to correspond to "
                            << currentFontFamily);

                    index = static_cast<decltype(index)>(
                        std::distance(fontNames.constBegin(), it));

                    break;
                }
            }
        }

        if (index < 0) {
            QNDEBUG(
                "widget::NoteEditorWidget",
                "Could not find neither exact nor approximate match for the "
                    << "font name " << fontFamily
                    << " within available font names: "
                    << fontNames.join(QStringLiteral(", "))
                    << ", will add the missing font name to the list");

            fontNames << fontFamily;
            fontNames.sort(Qt::CaseInsensitive);

            const auto it = std::lower_bound(
                fontNames.constBegin(), fontNames.constEnd(), fontFamily);

            if (it != fontNames.constEnd() && *it == fontFamily) {
                index = static_cast<decltype(index)>(
                    std::distance(fontNames.constBegin(), it));
            }
            else {
                index = fontNames.indexOf(fontFamily);
            }

            if (Q_UNLIKELY(index < 0)) {
                QNWARNING(
                    "widget::NoteEditorWidget",
                    "Something is weird: can't find the just added font "
                        << fontFamily << " within the list of available fonts "
                        << "where it should have been just added: "
                        << fontNames.join(QStringLiteral(", ")));

                index = 0;
            }

            m_limitedFontsListModel->setStringList(fontNames);
        }

        QObject::disconnect(
            m_ui->limitedFontComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);

        Q_ASSERT(index <= std::numeric_limits<int>::max());
        m_ui->limitedFontComboBox->setCurrentIndex(static_cast<int>(index));

        QNTRACE(
            "widget::NoteEditorWidget",
            "Set limited font combo box index to "
                << index << " corresponding to font family " << fontFamily);

        QObject::connect(
            m_ui->limitedFontComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);
    }
    else {
        QObject::disconnect(
            m_ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);

        m_ui->fontComboBox->setCurrentFont(currentFont);

        QNTRACE(
            "widget::NoteEditorWidget",
            "Font family from combo box: "
                << m_ui->fontComboBox->currentFont().family()
                << ", font family set by QFont's constructor from it: "
                << currentFont.family());

        QObject::connect(
            m_ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);
    }

    setupFontSizesForFont(currentFont);
}

void NoteEditorWidget::onEditorTextFontSizeChanged(const int fontSize)
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextFontSizeChanged: " << fontSize);

    if (m_lastSuggestedFontSize == fontSize) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "This font size has already been suggested previously");
        return;
    }

    m_lastSuggestedFontSize = fontSize;

    const int fontSizeIndex =
        m_ui->fontSizeComboBox->findData(QVariant{fontSize}, Qt::UserRole);

    if (fontSizeIndex >= 0) {
        m_lastFontSizeComboBoxIndex = fontSizeIndex;
        m_lastActualFontSize = fontSize;

        if (m_ui->fontSizeComboBox->currentIndex() != fontSizeIndex) {
            m_ui->fontSizeComboBox->setCurrentIndex(fontSizeIndex);
            QNTRACE(
                "widget::NoteEditorWidget",
                "fontSizeComboBox: set current index "
                    << "to " << fontSizeIndex
                    << ", found font size = " << fontSize);
        }

        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "Can't find font size "
            << fontSize
            << " within those listed in font size combobox, will try to choose "
            << "the closest one instead");

    const int numFontSizes = m_ui->fontSizeComboBox->count();
    int currentSmallestDiscrepancy = 1e5;
    int currentClosestIndex = -1;
    int currentClosestFontSize = -1;

    for (int i = 0; i < numFontSizes; ++i) {
        const auto value = m_ui->fontSizeComboBox->itemData(i, Qt::UserRole);
        bool conversionResult = false;
        const int valueInt = value.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING(
                "widget::NoteEditorWidget",
                "Can't convert value from font size combo box to int: "
                    << value);
            continue;
        }

        const int discrepancy = std::abs(valueInt - fontSize);
        if (currentSmallestDiscrepancy > discrepancy) {
            currentSmallestDiscrepancy = discrepancy;
            currentClosestIndex = i;
            currentClosestFontSize = valueInt;
            QNTRACE(
                "widget::NoteEditorWidget",
                "Updated current closest index to "
                    << i << ": font size = " << valueInt);
        }
    }

    if (currentClosestIndex < 0) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Couldn't find closest font size to " << fontSize);
        return;
    }

    QNTRACE(
        "widget::NoteEditorWidget",
        "Found closest current font size: "
            << currentClosestFontSize << ", index = " << currentClosestIndex);

    if (m_lastFontSizeComboBoxIndex == currentClosestIndex &&
        m_lastActualFontSize == currentClosestFontSize)
    {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Neither font size nor its index within the font combo box have "
            "changed");
        return;
    }

    m_lastFontSizeComboBoxIndex = currentClosestIndex;
    m_lastActualFontSize = currentClosestFontSize;

    if (m_ui->fontSizeComboBox->currentIndex() != currentClosestIndex) {
        m_ui->fontSizeComboBox->setCurrentIndex(currentClosestIndex);
    }
}

void NoteEditorWidget::onEditorTextInsertTableDialogRequested()
{
    QNTRACE(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorTextInsertTableDialogRequested");

    auto tableSettingsDialog = std::make_unique<TableSettingsDialog>(this);
    if (tableSettingsDialog->exec() == QDialog::Rejected) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Returned from TableSettingsDialog::exec: rejected");
        return;
    }

    QNTRACE(
        "widget::NoteEditorWidget",
        "Returned from TableSettingsDialog::exec: accepted");

    const int numRows = tableSettingsDialog->numRows();
    const int numColumns = tableSettingsDialog->numColumns();
    const double tableWidth = tableSettingsDialog->tableWidth();
    const bool relativeWidth = tableSettingsDialog->relativeWidth();

    if (relativeWidth) {
        m_ui->noteEditor->insertRelativeWidthTable(
            numRows, numColumns, tableWidth);
    }
    else {
        m_ui->noteEditor->insertFixedWidthTable(
            numRows, numColumns, static_cast<int>(tableWidth));
    }

    m_ui->noteEditor->setFocus();
}

void NoteEditorWidget::onEditorSpellCheckerNotReady()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorSpellCheckerNotReady");

    m_pendingEditorSpellChecker = true;

    Q_EMIT notifyError(ErrorString{
        QT_TR_NOOP("Spell checker is loading dictionaries, please wait")});
}

void NoteEditorWidget::onEditorSpellCheckerReady()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onEditorSpellCheckerReady");

    if (!m_pendingEditorSpellChecker) {
        return;
    }

    m_pendingEditorSpellChecker = false;

    // Send the empty message to remove the previous one about the non-ready
    // spell checker
    Q_EMIT notifyError(ErrorString{});
}

void NoteEditorWidget::onEditorHtmlUpdate(QString html)
{
    QNTRACE("widget::NoteEditorWidget", "NoteEditorWidget::onEditorHtmlUpdate");

    m_lastNoteEditorHtml = std::move(html);

    if (Q_LIKELY(m_ui->noteEditor->isModified())) {
        m_ui->saveNotePushButton->setEnabled(true);
    }

    if (!m_ui->noteSourceView->isVisible()) {
        return;
    }

    updateNoteSourceView(html);
}

void NoteEditorWidget::onFindInsideNoteAction()
{
    QNDEBUG(
        "widget::NoteEditorWidget", "NoteEditorWidget::onFindInsideNoteAction");

    if (m_ui->findAndReplaceWidget->isHidden()) {
        QString textToFind = m_ui->noteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_ui->findAndReplaceWidget->textToFind();
        }
        else {
            m_ui->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_ui->findAndReplaceWidget->setHidden(false);
        m_ui->findAndReplaceWidget->show();
    }

    onFindNextInsideNote(
        m_ui->findAndReplaceWidget->textToFind(),
        m_ui->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onFindPreviousInsideNoteAction()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFindPreviousInsideNoteAction");

    if (m_ui->findAndReplaceWidget->isHidden()) {
        QString textToFind = m_ui->noteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_ui->findAndReplaceWidget->textToFind();
        }
        else {
            m_ui->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_ui->findAndReplaceWidget->setHidden(false);
        m_ui->findAndReplaceWidget->show();
    }

    onFindPreviousInsideNote(
        m_ui->findAndReplaceWidget->textToFind(),
        m_ui->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onReplaceInsideNoteAction()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onReplaceInsideNoteAction");

    if (m_ui->findAndReplaceWidget->isHidden() ||
        !m_ui->findAndReplaceWidget->replaceEnabled())
    {
        QNTRACE(
            "widget::NoteEditorWidget",
            "At least the replacement part of find and replace widget is "
            "hidden, will only show it and do nothing else");

        QString textToFind = m_ui->noteEditor->selectedText();
        if (textToFind.isEmpty()) {
            textToFind = m_ui->findAndReplaceWidget->textToFind();
        }
        else {
            m_ui->findAndReplaceWidget->setTextToFind(textToFind);
        }

        m_ui->findAndReplaceWidget->setHidden(false);
        m_ui->findAndReplaceWidget->setReplaceEnabled(true);
        m_ui->findAndReplaceWidget->show();
        return;
    }

    onReplaceInsideNote(
        m_ui->findAndReplaceWidget->textToFind(),
        m_ui->findAndReplaceWidget->replacementText(),
        m_ui->findAndReplaceWidget->matchCase());
}

void NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage(
    const qevercloud::Note & note, const qevercloud::Notebook & notebook)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage: note local "
            << "id = " << note.localId() << ", notebook = "
            << notebook.localId());

    QNTRACE(
        "widget::NoteEditorWidget",
        "Note: " << note << "\nNotebook: " << notebook);

    QObject::disconnect(
        m_ui->noteEditor, &NoteEditor::noteAndNotebookFoundInLocalStorage,
        this, &NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage);

    QObject::disconnect(
        m_ui->noteEditor, &NoteEditor::noteNotFound, this,
        &NoteEditorWidget::onNoteNotFoundInLocalStorage);

    setNoteAndNotebook(note, notebook);

    QNTRACE(
        "widget::NoteEditorWidget",
        "Emitting resolved signal, note local id = " << m_noteLocalId);

    Q_EMIT resolved();
}

void NoteEditorWidget::onNoteNotFoundInLocalStorage(const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteNotFoundInLocalStorage: note local id = "
            << noteLocalId);

    clear();

    ErrorString errorDescription{
        QT_TR_NOOP("Can't display note: either note or its notebook were not "
                   "found within the local storage")};

    QNWARNING("widget::NoteEditorWidget", errorDescription);
    Q_EMIT notifyError(std::move(errorDescription));
}

void NoteEditorWidget::onFindAndReplaceWidgetClosed()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFindAndReplaceWidgetClosed");

    onFindNextInsideNote(QString{}, false);
}

void NoteEditorWidget::onTextToFindInsideNoteEdited(const QString & textToFind)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onTextToFindInsideNoteEdited: " << textToFind);

    const bool matchCase = m_ui->findAndReplaceWidget->matchCase();
    onFindNextInsideNote(textToFind, matchCase);
}

void NoteEditorWidget::onFindNextInsideNote(
    const QString & textToFind, const bool matchCase)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFindNextInsideNote: text to find = " << textToFind
            << ", match case = " << (matchCase ? "true" : "false"));

    if (Q_UNLIKELY(m_ui->findAndReplaceWidget->isHidden())) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Find and replace widget is not shown, nothing to do");
        return;
    }

    m_ui->noteEditor->findNext(textToFind, matchCase);
}

void NoteEditorWidget::onFindPreviousInsideNote(
    const QString & textToFind, const bool matchCase)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFindPreviousInsideNote: text to find = "
            << textToFind
            << ", match case = " << (matchCase ? "true" : "false"));

    if (Q_UNLIKELY(m_ui->findAndReplaceWidget->isHidden())) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Find and replace widget is not shown, nothing to do");
        return;
    }

    m_ui->noteEditor->findPrevious(textToFind, matchCase);
}

void NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged(
    const bool matchCase)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged: "
            << "match case = " << (matchCase ? "true" : "false"));

    if (Q_UNLIKELY(m_ui->findAndReplaceWidget->isHidden())) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Find and replace widget is not shown, nothing to do");
        return;
    }

    const QString textToFind = m_ui->findAndReplaceWidget->textToFind();
    m_ui->noteEditor->findNext(textToFind, matchCase);
}

void NoteEditorWidget::onReplaceInsideNote(
    const QString & textToReplace, const QString & replacementText,
    const bool matchCase)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onReplaceInsideNote: text to replace = "
            << textToReplace << ", replacement text = " << replacementText
            << ", match case = " << (matchCase ? "true" : "false"));

    if (Q_UNLIKELY(m_ui->findAndReplaceWidget->isHidden())) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Find and replace widget is not shown, nothing to do");
        return;
    }

    m_ui->findAndReplaceWidget->setReplaceEnabled(true);
    m_ui->noteEditor->replace(textToReplace, replacementText, matchCase);
}

void NoteEditorWidget::onReplaceAllInsideNote(
    const QString & textToReplace, const QString & replacementText,
    const bool matchCase)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onReplaceAllInsideNote: text to replace = "
            << textToReplace << ", replacement text = " << replacementText
            << ", match case = " << (matchCase ? "true" : "false"));

    if (Q_UNLIKELY(m_ui->findAndReplaceWidget->isHidden())) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Find and replace widget is not shown, nothing to do");
        return;
    }

    m_ui->findAndReplaceWidget->setReplaceEnabled(true);
    m_ui->noteEditor->replaceAll(textToReplace, replacementText, matchCase);
}

void NoteEditorWidget::updateNoteInLocalStorage()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::updateNoteInLocalStorage");

    if (!m_currentNote) {
        QNDEBUG("widget::NoteEditorWidget", "No note is set to the editor");
        return;
    }

    m_ui->saveNotePushButton->setEnabled(false);
    m_isNewNote = false;
    m_ui->noteEditor->saveNoteToLocalStorage();
}

void NoteEditorWidget::onResourcePut(const qevercloud::Resource & resource)
{
    if (!m_currentNote || !m_currentNotebook) {
        return;
    }

    if (resource.noteLocalId() != m_currentNote->localId()) {
        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onResourcePut: resource: " << resource);

    auto & mutableResources = m_currentNote->mutableResources();
    if (!mutableResources || mutableResources->isEmpty()) {
        // Note had no resources before, adding one
        m_currentNote->setResources(QList<qevercloud::Resource>{resource});
        return;
    }

    const auto it = std::find_if(
        mutableResources->begin(),
        mutableResources->end(),
        [&resource](const qevercloud::Resource & r) {
            return r.localId() == resource.localId();
        });
    if (it != mutableResources->end()) {
        *it = resource;
    }
    else {
        m_currentNote->setResources(QList<qevercloud::Resource>{resource});
    }
}

void NoteEditorWidget::onResourceMetadataPut(
    const qevercloud::Resource & resource)
{
    if (!m_currentNote || !m_currentNotebook) {
        return;
    }

    if (resource.noteLocalId() != m_currentNote->localId()) {
        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onResourceMetadataPut: resource = " << resource);

    auto & mutableResources = m_currentNote->mutableResources();
    if (!mutableResources || mutableResources->isEmpty()) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Resource which metadata was updated was not found in the current "
            "note");
        return;
    }

    const auto it = std::find_if(
        mutableResources->begin(),
        mutableResources->end(),
        [&resource](const qevercloud::Resource & r) {
            return r.localId() == resource.localId();
        });
    if (it != mutableResources->end()) {
        auto data = it->data();
        auto alternateData = it->alternateData();
        *it = resource;
        it->setData(std::move(data));
        it->setAlternateData(std::move(alternateData));
    }
    else {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Resource which metadata was updated was not found in the current "
            "note");
    }
}

void NoteEditorWidget::onResourceExpunged(const QString & resourceLocalId)
{
    if (!m_currentNote || !m_currentNotebook) {
        return;
    }

    auto & mutableResources = m_currentNote->mutableResources();
    if (!mutableResources || mutableResources->isEmpty()) {
        return;
    }

    const auto it = std::find_if(
        mutableResources->begin(),
        mutableResources->end(),
        [&resourceLocalId](const qevercloud::Resource & r) {
            return r.localId() == resourceLocalId;
        });
    if (it != mutableResources->end()) {
        mutableResources->erase(it);
    }
}

void NoteEditorWidget::onNotebookPut(const qevercloud::Notebook & notebook)
{
    if (!m_currentNote || !m_currentNotebook ||
        m_currentNotebook->localId() != notebook.localId())
    {
        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNotebookPut: " << notebook);

    setNoteAndNotebook(*m_currentNote, notebook);
}

void NoteEditorWidget::onNotebookExpunged(const QString & notebookLocalId)
{
    if (!m_currentNotebook ||
        m_currentNotebook->localId() != notebookLocalId)
    {
        return;
    }

    QNINFO(
        "widget::NoteEditorWidget",
        "The notebook containing the note loaded into "
            << "the editor was expunged from the local storage: "
            << *m_currentNotebook);

    clear();
    Q_EMIT invalidated();
}

void NoteEditorWidget::onPrintNoteButtonPressed()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onPrintNoteButtonPressed");

    ErrorString errorDescription;
    if (!printNote(errorDescription)) {
        Q_EMIT notifyError(std::move(errorDescription));
    }
}

void NoteEditorWidget::onExportNoteToPdfButtonPressed()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onExportNoteToPdfButtonPressed");

    ErrorString errorDescription;
    if (!exportNoteToPdf(errorDescription)) {
        Q_EMIT notifyError(std::move(errorDescription));
    }
}

void NoteEditorWidget::onExportNoteToEnexButtonPressed()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onExportNoteToEnexButtonPressed");

    ErrorString errorDescription;
    if (!exportNoteToEnex(errorDescription)) {
        if (errorDescription.isEmpty()) {
            return;
        }

        Q_EMIT notifyError(std::move(errorDescription));
    }
}

void NoteEditorWidget::createConnections()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::createConnections");

    auto * notifier = m_localStorage->notifier();

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteUpdated, this,
        [this](
            const qevercloud::Note & note,
            const local_storage::ILocalStorage::UpdateNoteOptions options) {
            const bool resourcesUpdated =
                options.testFlag(
                    local_storage::ILocalStorage::UpdateNoteOption::
                        UpdateResourceMetadata) &&
                options.testFlag(
                    local_storage::ILocalStorage::UpdateNoteOption::
                        UpdateResourceBinaryData);

            const bool tagsUpdated = options.testFlag(
                local_storage::ILocalStorage::UpdateNoteOption::UpdateTags);
            onNotePut(note, resourcesUpdated, tagsUpdated);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notePut, this,
        [this](const qevercloud::Note & note) {
            onNotePut(note, true, true);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteExpunged, this,
        [this](const QString & noteLocalId) {
            onNoteExpunged(noteLocalId);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::resourcePut, this,
        &NoteEditorWidget::onResourcePut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::resourceMetadataPut,
        this, &NoteEditorWidget::onResourceMetadataPut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::resourceExpunged, this,
        &NoteEditorWidget::onResourceExpunged);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookPut, this,
        &NoteEditorWidget::onNotebookPut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        &NoteEditorWidget::onNotebookExpunged);

    // Connect to font sizes combobox signals
    QObject::connect(
        m_ui->fontSizeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged,
        Qt::UniqueConnection);

    QObject::connect(
        this, &NoteEditorWidget::insertInAppNoteLink, m_ui->noteEditor,
        &NoteEditor::insertInAppNoteLink);

    // Connect to note tags widget's signals
    QObject::connect(
        m_ui->tagNameLabelsContainer, &NoteTagsWidget::noteTagsListChanged,
        this, &NoteEditorWidget::onNoteTagsListChanged);

    QObject::connect(
        m_ui->tagNameLabelsContainer,
        &NoteTagsWidget::newTagLineEditReceivedFocusFromWindowSystem, this,
        &NoteEditorWidget::onNewTagLineEditReceivedFocusFromWindowSystem);

    // Connect to note title updates
    QObject::connect(
        m_ui->noteNameLineEdit, &QLineEdit::textEdited, this,
        &NoteEditorWidget::onNoteTitleEdited);

    QObject::connect(
        m_ui->noteNameLineEdit, &QLineEdit::editingFinished, this,
        &NoteEditorWidget::onNoteTitleUpdated);

    // Connect note editor's signals to local signals and slots
    QObject::connect(
        m_ui->noteEditor, &NoteEditor::inAppNoteLinkClicked, this,
        &NoteEditorWidget::inAppNoteLinkClicked);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::inAppNoteLinkPasteRequested, this,
        &NoteEditorWidget::onEditorInAppLinkPasteRequested);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::convertedToNote, this,
        &NoteEditorWidget::onEditorNoteUpdate);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::cantConvertToNote, this,
        &NoteEditorWidget::onEditorNoteUpdateFailed);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::noteSavedToLocalStorage, this,
        &NoteEditorWidget::onNoteSavedToLocalStorage);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::noteEditorHtmlUpdated, this,
        &NoteEditorWidget::onEditorHtmlUpdate);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::noteLoaded, this,
        &NoteEditorWidget::noteLoaded);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::noteModified, this,
        &NoteEditorWidget::onNoteEditorModified);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textBoldState, this,
        &NoteEditorWidget::onEditorTextBoldStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textItalicState, this,
        &NoteEditorWidget::onEditorTextItalicStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textUnderlineState, this,
        &NoteEditorWidget::onEditorTextUnderlineStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textUnderlineState, this,
        &NoteEditorWidget::onEditorTextUnderlineStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textStrikethroughState, this,
        &NoteEditorWidget::onEditorTextStrikethroughStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textAlignLeftState, this,
        &NoteEditorWidget::onEditorTextAlignLeftStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textAlignCenterState, this,
        &NoteEditorWidget::onEditorTextAlignCenterStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textAlignRightState, this,
        &NoteEditorWidget::onEditorTextAlignRightStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textAlignFullState, this,
        &NoteEditorWidget::onEditorTextAlignFullStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textInsideOrderedListState, this,
        &NoteEditorWidget::onEditorTextInsideOrderedListStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textInsideUnorderedListState, this,
        &NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textInsideTableState, this,
        &NoteEditorWidget::onEditorTextInsideTableStateChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textFontFamilyChanged, this,
        &NoteEditorWidget::onEditorTextFontFamilyChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::textFontSizeChanged, this,
        &NoteEditorWidget::onEditorTextFontSizeChanged);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::insertTableDialogRequested, this,
        &NoteEditorWidget::onEditorTextInsertTableDialogRequested);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::spellCheckerNotReady, this,
        &NoteEditorWidget::onEditorSpellCheckerNotReady);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::spellCheckerReady, this,
        &NoteEditorWidget::onEditorSpellCheckerReady);

    QObject::connect(
        m_ui->noteEditor, &NoteEditor::notifyError, this,
        &NoteEditorWidget::notifyError);

    // Connect find and replace widget actions to local slots
    QObject::connect(
        m_ui->findAndReplaceWidget, &FindAndReplaceWidget::closed, this,
        &NoteEditorWidget::onFindAndReplaceWidgetClosed);

    QObject::connect(
        m_ui->findAndReplaceWidget, &FindAndReplaceWidget::textToFindEdited,
        this, &NoteEditorWidget::onTextToFindInsideNoteEdited);

    QObject::connect(
        m_ui->findAndReplaceWidget, &FindAndReplaceWidget::findNext, this,
        &NoteEditorWidget::onFindNextInsideNote);

    QObject::connect(
        m_ui->findAndReplaceWidget, &FindAndReplaceWidget::findPrevious, this,
        &NoteEditorWidget::onFindPreviousInsideNote);

    QObject::connect(
        m_ui->findAndReplaceWidget,
        &FindAndReplaceWidget::searchCaseSensitivityChanged, this,
        &NoteEditorWidget::onFindInsideNoteCaseSensitivityChanged);

    QObject::connect(
        m_ui->findAndReplaceWidget, &FindAndReplaceWidget::replace, this,
        &NoteEditorWidget::onReplaceInsideNote);

    QObject::connect(
        m_ui->findAndReplaceWidget, &FindAndReplaceWidget::replaceAll, this,
        &NoteEditorWidget::onReplaceAllInsideNote);

    // Connect toolbar buttons actions to local slots
    QObject::connect(
        m_ui->fontBoldPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextBoldToggled);

    QObject::connect(
        m_ui->fontItalicPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextItalicToggled);

    QObject::connect(
        m_ui->fontUnderlinePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextUnderlineToggled);

    QObject::connect(
        m_ui->fontStrikethroughPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextStrikethroughToggled);

    QObject::connect(
        m_ui->formatJustifyLeftPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignLeftAction);

    QObject::connect(
        m_ui->formatJustifyCenterPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignCenterAction);

    QObject::connect(
        m_ui->formatJustifyRightPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignRightAction);

    QObject::connect(
        m_ui->formatJustifyFullPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAlignFullAction);

    QObject::connect(
        m_ui->insertHorizontalLinePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextAddHorizontalLineAction);

    QObject::connect(
        m_ui->formatIndentMorePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextIncreaseIndentationAction);

    QObject::connect(
        m_ui->formatIndentLessPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextDecreaseIndentationAction);

    QObject::connect(
        m_ui->formatListUnorderedPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextInsertUnorderedListAction);

    QObject::connect(
        m_ui->formatListOrderedPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextInsertOrderedListAction);

    QObject::connect(
        m_ui->formatAsSourceCodePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorTextFormatAsSourceCodeAction);

    QObject::connect(
        m_ui->chooseTextColorToolButton, &ColorPickerToolButton::colorSelected,
        this, &NoteEditorWidget::onEditorChooseTextColor);

    QObject::connect(
        m_ui->chooseBackgroundColorToolButton,
        &ColorPickerToolButton::colorSelected, this,
        &NoteEditorWidget::onEditorChooseBackgroundColor);

    QObject::connect(
        m_ui->spellCheckBox, &QCheckBox::stateChanged, this,
        &NoteEditorWidget::onEditorSpellCheckStateChanged);

    QObject::connect(
        m_ui->insertToDoCheckboxPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onEditorInsertToDoCheckBoxAction);

    QObject::connect(
        m_ui->insertTableToolButton, &InsertTableToolButton::createdTable,
        this, &NoteEditorWidget::onEditorInsertTable);

    QObject::connect(
        m_ui->printNotePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onPrintNoteButtonPressed);

    QObject::connect(
        m_ui->exportNoteToPdfPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onExportNoteToPdfButtonPressed);

    QObject::connect(
        m_ui->exportNoteToEnexPushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onExportNoteToEnexButtonPressed);

    // Connect toolbar button actions to editor slots
    QObject::connect(
        m_ui->copyPushButton, &QPushButton::clicked, m_ui->noteEditor,
        &NoteEditor::copy);

    QObject::connect(
        m_ui->cutPushButton, &QPushButton::clicked, m_ui->noteEditor,
        &NoteEditor::cut);

    QObject::connect(
        m_ui->pastePushButton, &QPushButton::clicked, m_ui->noteEditor,
        &NoteEditor::paste);

    QObject::connect(
        m_ui->undoPushButton, &QPushButton::clicked, m_ui->noteEditor,
        &NoteEditor::undo);

    QObject::connect(
        m_ui->redoPushButton, &QPushButton::clicked, m_ui->noteEditor,
        &NoteEditor::redo);

    QObject::connect(
        m_ui->saveNotePushButton, &QPushButton::clicked, this,
        &NoteEditorWidget::onSaveNoteAction);
}

void NoteEditorWidget::clear()
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::clear: note "
            << (m_currentNote ? m_currentNote->localId()
                              : QStringLiteral("<null>")));

    m_currentNote.reset();
    m_currentNotebook.reset();
    m_pendingFindingCurrentNotebook = false;

    QObject::disconnect(
        m_ui->noteEditor, &NoteEditor::noteAndNotebookFoundInLocalStorage,
        this, &NoteEditorWidget::onFoundNoteAndNotebookInLocalStorage);

    QObject::disconnect(
        m_ui->noteEditor, &NoteEditor::noteNotFound, this,
        &NoteEditorWidget::onNoteNotFoundInLocalStorage);

    m_ui->noteEditor->clear();

    m_ui->tagNameLabelsContainer->clear();
    m_ui->noteNameLineEdit->clear();

    m_lastNoteTitleOrPreviewText.clear();

    m_pendingEditorSpellChecker = false;
    m_currentNoteWasExpunged = false;
    m_noteHasBeenModified = false;
}

void NoteEditorWidget::onNotePut(
    const qevercloud::Note & note, const bool resourcesUpdated,
    const bool tagsUpdated)
{
    if (Q_UNLIKELY(
            !m_currentNote || m_currentNote->localId() != note.localId()))
    {
        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNotePut: note local id = "
            << note.localId()
            << ", resources updated = " << (resourcesUpdated ? "true" : "false")
            << ", tags updated = " << (tagsUpdated ? "true" : "false"));

    std::optional<QList<qevercloud::Resource>> backupResources;
    if (!resourcesUpdated) {
        backupResources = m_currentNote->resources();
    }

    QStringList backupTagLocalIds;
    if (!tagsUpdated) {
        backupTagLocalIds = m_currentNote->tagLocalIds();
    }

    std::optional<QList<qevercloud::Guid>> backupTagGuids;
    if (!tagsUpdated) {
        backupTagGuids = m_currentNote->tagGuids();
    }

    std::optional<QString> backupTitle;
    if (m_noteTitleIsEdited) {
        backupTitle = m_currentNote->title();
    }

    *m_currentNote = note;

    if (!resourcesUpdated) {
        m_currentNote->setResources(std::move(backupResources));
    }

    if (!tagsUpdated) {
        m_currentNote->setTagLocalIds(std::move(backupTagLocalIds));
        m_currentNote->setTagGuids(std::move(backupTagGuids));
    }

    if (m_noteTitleIsEdited) {
        m_currentNote->setTitle(std::move(backupTitle));
    }

    if (note.deleted()) {
        QNINFO(
            "widget::NoteEditorWidget",
            "The note loaded into the editor was deleted: " << *m_currentNote);
        Q_EMIT invalidated();
        return;
    }

    if (Q_UNLIKELY(!m_currentNotebook)) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Current notebook is null - a bit unexpected at this point");

        if (m_pendingFindingCurrentNotebook) {
            QNDEBUG(
                "widget::NoteEditorWidget",
                "Already waiting for current notebook");
            return;
        }

        const auto * cachedNotebook =
            m_notebookCache.get(m_currentNote->notebookLocalId());
        if (cachedNotebook) {
            m_currentNotebook.emplace(*cachedNotebook);
        }
        else {
            m_pendingFindingCurrentNotebook = true;

            auto findNotebookFuture = m_localStorage->findNotebookByLocalId(
                m_currentNote->notebookLocalId());

            auto findNotebookThenFuture = threading::then(
                std::move(findNotebookFuture), this,
                [this, noteLocalId = m_currentNote->localId(),
                 notebookLocalId = m_currentNote->notebookLocalId()](
                    const std::optional<qevercloud::Notebook> & notebook) {
                    if (Q_UNLIKELY(
                            !m_currentNote ||
                            m_currentNote->localId() != noteLocalId))
                    {
                        return;
                    }

                    m_pendingFindingCurrentNotebook = false;

                    if (Q_UNLIKELY(!notebook)) {
                        QNDEBUG(
                            "widget::NoteEditorWidget",
                            "Could not find notebook for current note: "
                                << "notebook local id = " << notebookLocalId);

                        clear();

                        Q_EMIT notifyError(ErrorString{QT_TR_NOOP(
                            "Cannot find notebook of the editor's note")});
                        return;
                    }

                    QNDEBUG(
                        "widget::NoteEditorWidget",
                        "Found notebook for current note: " << *notebook);

                    m_currentNotebook.emplace(*notebook);
                    setNoteAndNotebook(*m_currentNote, *m_currentNotebook);

                    QNDEBUG(
                        "widget::NoteEditorWidget",
                        "Emitting resolved signal for note " << noteLocalId);
                    Q_EMIT resolved();
                });

            threading::onFailed(
                std::move(findNotebookThenFuture), this,
                [this, noteLocalId = m_currentNote->localId(),
                 notebookLocalId =
                     m_currentNote->notebookLocalId()](const QException & e) {
                    const auto message = exceptionMessage(e);

                    QNWARNING(
                        "widget::NoteEditorWidget",
                        "Could not find notebook for current note: "
                            << "notebook local id = " << notebookLocalId << ": "
                            << message);

                    clear();
                    Q_EMIT notifyError(ErrorString{QT_TR_NOOP(
                        "Cannot find notebook of the editor's note")});
                });
        }
    }

    setNoteAndNotebook(*m_currentNote, *m_currentNotebook);
}

void NoteEditorWidget::onNoteExpunged(const QString & noteLocalId)
{
    if (!m_currentNote || (m_currentNote->localId() != noteLocalId)) {
        return;
    }

    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::onNoteExpunged: note local id = " << noteLocalId);

    m_currentNoteWasExpunged = true;

    // TODO: ideally should allow the choice to either re-save the note or to
    // drop it

    QNINFO(
        "widget::NoteEditorWidget",
        "The note loaded into the editor was expunged from the local storage: "
            << *m_currentNote);

    Q_EMIT invalidated();
}

void NoteEditorWidget::setupSpecialIcons()
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::setupSpecialIcons");

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
            "widget::NoteEditorWidget",
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

        m_ui->exportNoteToEnexPushButton->setIcon(fallbackIcon);
    }
    else {
        QIcon themeIcon = QIcon::fromTheme(enexIconName);
        m_ui->exportNoteToEnexPushButton->setIcon(themeIcon);
    }
}

void NoteEditorWidget::onNoteEditorColorsUpdate()
{
    setupNoteEditorColors();

    if (noteLocalId().isEmpty()) {
        m_ui->noteEditor->clear();
        return;
    }

    if (!m_currentNote || !m_currentNotebook) {
        QNTRACE(
            "widget::NoteEditorWidget",
            "Current note or notebook was not found yet");
        return;
    }

    if (isModified()) {
        ErrorString errorDescription;
        auto status = checkAndSaveModifiedNote(errorDescription);
        if (status == NoteSaveStatus::Failed) {
            QNWARNING(
                "widget::NoteEditorWidget",
                "Failed to save modified note: " << errorDescription);
            return;
        }

        if (status == NoteSaveStatus::Timeout) {
            QNWARNING(
                "widget::NoteEditorWidget",
                "Failed to save modified note in "
                    << "due time");
            return;
        }
    }
}

void NoteEditorWidget::setupFontsComboBox()
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::setupFontsComboBox");

    if (useLimitedSetOfFonts()) {
        m_ui->fontComboBox->setHidden(true);
        m_ui->fontComboBox->setDisabled(true);
        setupLimitedFontsComboBox();
    }
    else {
        m_ui->limitedFontComboBox->setHidden(true);
        m_ui->limitedFontComboBox->setDisabled(true);

        QObject::connect(
            m_ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &NoteEditorWidget::onFontComboBoxFontChanged);
    }

    setupNoteEditorDefaultFont();
}

void NoteEditorWidget::setupLimitedFontsComboBox(const QString & startupFont)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
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

    delete m_limitedFontsListModel;
    m_limitedFontsListModel = new QStringListModel{this};
    m_limitedFontsListModel->setStringList(limitedFontNames);
    m_ui->limitedFontComboBox->setModel(m_limitedFontsListModel);

    auto currentIndex = limitedFontNames.indexOf(startupFont);
    if (currentIndex < 0) {
        currentIndex = 0;
    }

    Q_ASSERT(currentIndex <= std::numeric_limits<int>::max());
    m_ui->limitedFontComboBox->setCurrentIndex(static_cast<int>(currentIndex));

    auto * delegate = qobject_cast<LimitedFontsDelegate *>(
        m_ui->limitedFontComboBox->itemDelegate());

    if (!delegate) {
        delegate = new LimitedFontsDelegate(m_ui->limitedFontComboBox);
        m_ui->limitedFontComboBox->setItemDelegate(delegate);
    }

    QObject::connect(
        m_ui->limitedFontComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onLimitedFontsComboBoxCurrentIndexChanged);
}

void NoteEditorWidget::setupFontSizesComboBox()
{
    QNDEBUG(
        "widget::NoteEditorWidget", "NoteEditorWidget::setupFontSizesComboBox");

    QFont currentFont;
    if (!m_ui->limitedFontComboBox->isHidden()) {
        currentFont.setFamily(m_ui->limitedFontComboBox->currentText());
    }
    else {
        currentFont = m_ui->fontComboBox->currentFont();
    }

    setupFontSizesForFont(currentFont);
}

void NoteEditorWidget::setupFontSizesForFont(const QFont & font)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::setupFontSizesForFont: "
            << "family = " << font.family()
            << ", style name = " << font.styleName());

    QFontDatabase fontDatabase;
    auto fontSizes = fontDatabase.pointSizes(font.family(), font.styleName());
    if (fontSizes.isEmpty()) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "Couldn't find point sizes for font family "
                << font.family() << ", will use standard sizes instead");

        fontSizes = fontDatabase.standardSizes();
    }

    int currentFontSize = -1;
    int currentFontSizeIndex = m_ui->fontSizeComboBox->currentIndex();

    QList<int> currentFontSizes;
    const int currentCount = m_ui->fontSizeComboBox->count();
    currentFontSizes.reserve(currentCount);
    for (int i = 0; i < currentCount; ++i) {
        bool conversionResult = false;
        const QVariant data = m_ui->fontSizeComboBox->itemData(i);
        const int fontSize = data.toInt(&conversionResult);
        if (conversionResult) {
            currentFontSizes << fontSize;
            if (i == currentFontSizeIndex) {
                currentFontSize = fontSize;
            }
        }
    }

    if (currentFontSizes == fontSizes) {
        QNDEBUG(
            "widget::NoteEditorWidget",
            "No need to update the items within font sizes combo box: none of "
            "them have changed");
        return;
    }

    QObject::disconnect(
        m_ui->fontSizeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged);

    m_lastFontSizeComboBoxIndex = 0;
    m_ui->fontSizeComboBox->clear();
    const auto numFontSizes = fontSizes.size();

    QNTRACE(
        "widget::NoteEditorWidget",
        "Found " << numFontSizes << " font sizes for "
                 << "font family " << font.family());

    for (int i = 0; i < numFontSizes; ++i) {
        m_ui->fontSizeComboBox->addItem(
            QString::number(fontSizes[i]), QVariant{fontSizes[i]});

        QNTRACE(
            "widget::NoteEditorWidget",
            "Added item " << fontSizes[i] << "pt for index " << i);
    }

    m_lastFontSizeComboBoxIndex = -1;

    bool setFontSizeIndex = false;
    if (currentFontSize > 0) {
        const auto fontSizeIndex = fontSizes.indexOf(currentFontSize);
        if (fontSizeIndex >= 0) {
            Q_ASSERT(fontSizeIndex <= std::numeric_limits<int>::max());
            m_ui->fontSizeComboBox->setCurrentIndex(
                static_cast<int>(fontSizeIndex));
            QNTRACE(
                "widget::NoteEditorWidget",
                "Setting the current font size to its previous value: "
                    << currentFontSize);
            setFontSizeIndex = true;
        }
    }

    if (!setFontSizeIndex && currentFontSize != 12) {
        // Try to look for font size 12 as the sanest default font size
        const auto fontSizeIndex = fontSizes.indexOf(12);
        if (fontSizeIndex >= 0) {
            Q_ASSERT(fontSizeIndex <= std::numeric_limits<int>::max());
            m_ui->fontSizeComboBox->setCurrentIndex(
                static_cast<int>(fontSizeIndex));
            QNTRACE(
                "widget::NoteEditorWidget",
                "Setting the current font size to the default value of 12");
            setFontSizeIndex = true;
        }
    }

    if (!setFontSizeIndex) {
        // Try to find any font size between 10 and 20, should be good enough
        for (int i = 0; i < numFontSizes; ++i) {
            const int fontSize = fontSizes[i];
            if ((fontSize >= 10) && (fontSize <= 20)) {
                m_ui->fontSizeComboBox->setCurrentIndex(i);
                QNTRACE(
                    "widget::NoteEditorWidget",
                    "Setting the current font size to the default value of "
                        << fontSize);
                setFontSizeIndex = true;
                break;
            }
        }
    }

    if (!setFontSizeIndex && !fontSizes.isEmpty()) {
        // All attempts to pick some sane font size have failed,
        // will just take the median (or only) font size
        const auto index = numFontSizes / 2;
        Q_ASSERT(index <= std::numeric_limits<int>::max());
        m_ui->fontSizeComboBox->setCurrentIndex(static_cast<int>(index));
        QNTRACE(
            "widget::NoteEditorWidget",
            "Setting the current font size to the median value of "
                << fontSizes.at(index));
    }

    QObject::connect(
        m_ui->fontSizeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &NoteEditorWidget::onFontSizesComboBoxCurrentIndexChanged,
        Qt::UniqueConnection);
}

bool NoteEditorWidget::useLimitedSetOfFonts() const
{
    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    bool useLimitedFonts = false;
    if (appSettings.contains(preferences::keys::noteEditorUseLimitedSetOfFonts))
    {
        useLimitedFonts =
            appSettings.value(preferences::keys::noteEditorUseLimitedSetOfFonts)
                .toBool();

        QNDEBUG(
            "widget::NoteEditorWidget",
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
        "widget::NoteEditorWidget", "NoteEditorWidget::setupNoteEditorDefaultFont");

    const bool useLimitedFonts = !m_ui->limitedFontComboBox->isHidden();

    int pointSize = -1;
    const int fontSizeIndex = m_ui->fontSizeComboBox->currentIndex();
    if (fontSizeIndex >= 0) {
        bool conversionResult = false;
        QVariant fontSizeData =
            m_ui->fontSizeComboBox->itemData(fontSizeIndex);
        pointSize = fontSizeData.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING(
                "widget::NoteEditorWidget",
                "Failed to convert current font "
                    << "size to int: " << fontSizeData);
            pointSize = -1;
        }
    }

    QFont currentFont;
    if (useLimitedFonts) {
        QString fontFamily = m_ui->limitedFontComboBox->currentText();
        currentFont = QFont{fontFamily, pointSize};
    }
    else {
        QFont font = m_ui->fontComboBox->currentFont();
        currentFont = QFont{font.family(), pointSize};
    }

    m_ui->noteEditor->setDefaultFont(currentFont);
}

void NoteEditorWidget::setupNoteEditorColors()
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::setupNoteEditorColors");

    QPalette pal;

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    const QString fontColorName =
        appSettings.value(preferences::keys::noteEditorFontColor).toString();

    QColor fontColor{fontColorName};
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

    QColor highlightColor{highlightColorName};
    if (highlightColor.isValid()) {
        pal.setColor(QPalette::Highlight, highlightColor);
    }

    QString highlightedTextColorName =
        appSettings.value(preferences::keys::noteEditorHighlightedTextColor)
            .toString();

    QColor highlightedTextColor{highlightedTextColorName};
    if (highlightedTextColor.isValid()) {
        pal.setColor(QPalette::HighlightedText, highlightedTextColor);
    }

    appSettings.endGroup();

    m_ui->noteEditor->setDefaultPalette(pal);
}

void NoteEditorWidget::updateNoteSourceView(const QString & html)
{
    m_ui->noteSourceView->setPlainText(html);
}

void NoteEditorWidget::setNoteAndNotebook(
    const qevercloud::Note & note, const qevercloud::Notebook & notebook)
{
    QNDEBUG(
        "widget::NoteEditorWidget",
        "NoteEditorWidget::setCurrentNoteAndNotebook: note local id = "
            << note.localId()
            << ", notebook local id = " << notebook.localId());

    QNTRACE(
        "widget::NoteEditorWidget",
        "Note: " << note << "\nNotebook: " << notebook);

    if (!m_currentNote) {
        m_currentNote.emplace(note);
    }
    else {
        *m_currentNote = note;
    }

    if (!m_currentNotebook) {
        m_currentNotebook.emplace(notebook);
    }
    else {
        *m_currentNotebook = notebook;
    }

    m_ui->noteNameLineEdit->show();
    m_ui->tagNameLabelsContainer->show();

    if (!m_noteTitleIsEdited && note.title()) {
        QString title = *note.title();
        m_ui->noteNameLineEdit->setText(title);
        if (m_lastNoteTitleOrPreviewText != title) {
            m_lastNoteTitleOrPreviewText = std::move(title);
            Q_EMIT titleOrPreviewChanged(m_lastNoteTitleOrPreviewText);
        }
    }
    else if (!m_noteTitleIsEdited) {
        m_ui->noteNameLineEdit->clear();

        QString previewText;
        if (note.content()) {
            previewText = noteContentToPlainText(*note.content());
            previewText.truncate(140);
        }

        if (previewText != m_lastNoteTitleOrPreviewText) {
            m_lastNoteTitleOrPreviewText = std::move(previewText);
            Q_EMIT titleOrPreviewChanged(m_lastNoteTitleOrPreviewText);
        }
    }

    m_ui->noteEditor->setCurrentNoteLocalId(note.localId());
    m_ui->tagNameLabelsContainer->setCurrentNoteAndNotebook(note, notebook);

    m_ui->printNotePushButton->setDisabled(false);
    m_ui->exportNoteToPdfPushButton->setDisabled(false);
    m_ui->exportNoteToEnexPushButton->setDisabled(false);
}

QString NoteEditorWidget::blankPageHtml() const
{
    QString html;
    QTextStream strm{&html};

    strm << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" "
            "\"http://www.w3.org/TR/html4/strict.dtd\">"
            "<html><head>"
            "<meta http-equiv=\"Content-Type\" content=\"text/html\" "
            "charset=\"UTF-8\" />"
            "<style>"
            "body {"
            "background-color: ";

    const QColor backgroundColor = palette().color(QPalette::Window).darker(115);
    strm << backgroundColor.name();

    strm << ";color: ";
    const QColor foregroundColor = palette().color(QPalette::WindowText);
    strm << foregroundColor.name();

    strm << ";"
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
            "<div class=\"inner\">\n\n\n";

    strm << tr("Please select some existing note or create a new one");
    strm << "</div></div></div></body></html>";
    strm.flush();

    return html;
}

void NoteEditorWidget::setupBlankEditor()
{
    QNDEBUG("widget::NoteEditorWidget", "NoteEditorWidget::setupBlankEditor");

    m_ui->printNotePushButton->setDisabled(true);
    m_ui->exportNoteToPdfPushButton->setDisabled(true);
    m_ui->exportNoteToEnexPushButton->setDisabled(true);

    m_ui->noteNameLineEdit->hide();
    m_ui->tagNameLabelsContainer->hide();

    const QString initialHtml = blankPageHtml();
    m_ui->noteEditor->setInitialPageHtml(initialHtml);

    m_ui->findAndReplaceWidget->setHidden(true);
    m_ui->noteSourceView->setHidden(true);
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
        QNTRACE("widget::NoteEditorWidget", "Removed apostrophes: " << str);
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
    switch (status)
    {
    case NoteEditorWidget::NoteSaveStatus::Ok:
        dbg << "Ok";
        break;
    case NoteEditorWidget::NoteSaveStatus::Failed:
        dbg << "Failed";
        break;
    case NoteEditorWidget::NoteSaveStatus::Timeout:
        dbg << "Timeout";
        break;
    }

    return dbg;
}

} // namespace quentier

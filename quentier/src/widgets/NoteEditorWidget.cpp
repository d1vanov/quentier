#include "NoteEditorWidget.h"

// Doh, Qt Designer's inability to work with namespaces in the expected way
// is deeply disappointing
#include "NoteTagsWidget.h"
#include "FindAndReplaceWidget.h"
#include "../insert-table-tool-button/InsertTableToolButton.h"
#include "../insert-table-tool-button/TableSettingsDialog.h"
#include "../color-picker-tool-button/ColorPickerToolButton.h"
#include <quentier/note_editor/NoteEditor.h>
using quentier::FindAndReplaceWidget;
using quentier::NoteEditor;
using quentier::NoteTagsWidget;
#include "ui_NoteEditorWidget.h"

#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ResourceAdapter.h>
#include <QFontDatabase>
#include <QScopedPointer>

#define CHECK_NOTE_SET() \
    if (Q_UNLIKELY(m_pCurrentNote.isNull()) { \
        emit notifyError(QT_TR_NOOP("No note is set to the editor")); \
        return; \
    }

namespace quentier {

NoteEditorWidget::NoteEditorWidget(LocalStorageManagerThreadWorker & localStorageWorker,
                                   NoteCache & noteCache, NotebookCache & notebookCache,
                                   TagCache & tagCache, QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::NoteEditorWidget),
    m_noteCache(noteCache),
    m_notebookCache(notebookCache),
    m_tagCache(tagCache),
    m_pCurrentNote(),
    m_pCurrentNotebook(),
    m_findCurrentNoteRequestIds(),
    m_findCurrentNotebookRequestIds(),
    m_lastFontSizeComboBoxIndex(-1),
    m_lastFontComboBoxFontFamily(),
    m_lastSuggestedFontSize(-1),
    m_lastActualFontSize(-1),
    m_pendingEditorSpellChecker(false)
{
    m_pUi->setupUi(this);
    createConnections(localStorageWorker);
}

NoteEditorWidget::~NoteEditorWidget()
{
    delete m_pUi;
}

void NoteEditorWidget::setNoteLocalUid(const QString & noteLocalUid)
{
    QNDEBUG("NoteEditorWidget::setNoteLocalUid: " << noteLocalUid);

    if (!m_pCurrentNote.isNull() && (m_pCurrentNote->localUid() == noteLocalUid)) {
        QNDEBUG("This note is already set to the editor, nothing to do");
        return;
    }

    m_pUi->noteEditor->clear();
    m_pCurrentNote.reset(Q_NULLPTR);
    m_pCurrentNotebook.reset(Q_NULLPTR);

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        return;
    }

    const Note * pCachedNote = m_noteCache.get(noteLocalUid);

    // The cache might contain the note without resource binary data, need to check for this
    bool hasMissingResources = false;
    if (Q_LIKELY(pCachedNote))
    {
        QList<ResourceAdapter> resourceAdapters = pCachedNote->resourceAdapters();
        if (!resourceAdapters.isEmpty())
        {
            for(int i = 0, size = resourceAdapters.size(); i < size; ++i)
            {
                const ResourceAdapter & resourceAdapter = resourceAdapters[i];
                if (resourceAdapter.hasDataHash() && !resourceAdapter.hasDataBody()) {
                    hasMissingResources = true;
                    break;
                }
            }
        }
    }

    if (Q_UNLIKELY(!pCachedNote || hasMissingResources))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findCurrentNoteRequestIds.insert(requestId))
        Note dummy;
        dummy.setLocalUid(noteLocalUid);
        QNTRACE("Emitting the request to find the current note: local uid = " << noteLocalUid
                << ", request id = " << requestId);
        emit findNote(dummy, /* with resource binary data = */ true, requestId);
        return;
    }

    QNTRACE("Found the cached note");
    if (Q_UNLIKELY(!pCachedNote->hasNotebookLocalUid() && !pCachedNote->hasNotebookGuid())) {
        emit notifyError(QT_TR_NOOP("Can't set the note to the editor: the note has no linkage to any notebook"));
        return;
    }

    m_pCurrentNote.reset(new Note(*pCachedNote));

    const Notebook * pCachedNotebook = Q_NULLPTR;
    if (m_pCurrentNote->hasNotebookLocalUid()) {
        pCachedNotebook = m_notebookCache.get(m_pCurrentNote->notebookLocalUid());
    }

    if (Q_UNLIKELY(!pCachedNotebook))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findCurrentNotebookRequestIds.insert(requestId))
        Notebook dummy;
        if (m_pCurrentNote->hasNotebookLocalUid()) {
            dummy.setLocalUid(m_pCurrentNote->notebookLocalUid());
        }
        else {
            dummy.setLocalUid(QString());
            dummy.setGuid(m_pCurrentNote->notebookGuid());
        }

        QNTRACE("Emitting the request to find the current notebook: " << dummy
                << "\nRequest id = " << requestId);
        emit findNotebook(dummy, requestId);
        return;
    }

    // TODO: continue from here
}

void NoteEditorWidget::onEditorTextBoldToggled()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextItalicToggled()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextUnderlineToggled()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextStrikethroughToggled()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextAlignLeftAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextAlignCenterAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextAlignRightAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextAddHorizontalLineAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextIncreaseFontSizeAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextDecreaseFontSizeAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextIncreaseIndentationAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextDecreaseIndentationAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextInsertUnorderedListAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorTextInsertOrderedListAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorChooseTextColor(QColor color)
{
    Q_UNUSED(color)
    // TODO: implement
}

void NoteEditorWidget::onEditorChooseBackgroundColor(QColor color)
{
    Q_UNUSED(color)
    // TODO: implement
}

void NoteEditorWidget::onEditorSpellCheckStateChanged(int state)
{
    Q_UNUSED(state)
    // TODO: implement
}

void NoteEditorWidget::onEditorInsertToDoCheckBoxAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorInsertTableDialogAction()
{
    // TODO: implement
}

void NoteEditorWidget::onEditorInsertTable(int rows, int columns, double width, bool relativeWidth)
{
    Q_UNUSED(rows)
    Q_UNUSED(columns)
    Q_UNUSED(width)
    Q_UNUSED(relativeWidth)
    // TODO: implement
}

void NoteEditorWidget::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                          QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNoteFailed(Note note, bool withResourceBinaryData, QNLocalizedString errorDescription,
                                        QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onExpungeNoteComplete(Note note, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onEditorNoteUpdate(Note note)
{
    Q_UNUSED(note)
    // TODO: implement
}

void NoteEditorWidget::onEditorNoteUpdateFailed(QNLocalizedString error)
{
    QNDEBUG("NoteEditorWidget::onEditorNoteUpdateFailed: " << error);

    emit notifyError(error);
}

void NoteEditorWidget::onEditorTextBoldStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextBoldStateChanged: " << (state ? "enabled" : "disabled"));
    m_pUi->fontBoldPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextItalicStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextItalicStateChanged: " << (state ? "enabled" : "disabled"));
    m_pUi->fontItalicPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextUnderlineStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextUnderlineStateChanged: " << (state ? "enabled" : "disabled"));
    m_pUi->fontUnderlinePushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextStrikethroughStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextStrikethroughStateChanged: " << (state ? "enabled" : "disabled"));
    m_pUi->fontStrikethroughPushButton->setChecked(state);
}

void NoteEditorWidget::onEditorTextAlignLeftStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextAlignLeftStateChanged: " << (state ? "enabled" : "disabled"));

    m_pUi->formatJustifyLeftPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignCenterStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextAlignCenterStateChanged: " << (state ? "enabled" : "disabled"));

    m_pUi->formatJustifyCenterPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyRightPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextAlignRightStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextAlignRightStateChanged: " << (state ? "enabled" : "disabled"));

    m_pUi->formatJustifyRightPushButton->setChecked(state);

    if (state) {
        m_pUi->formatJustifyLeftPushButton->setChecked(false);
        m_pUi->formatJustifyCenterPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideOrderedListStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextInsideOrderedListStateChanged: " << (state ? "enabled" : "disabled"));

    m_pUi->formatListOrderedPushButton->setChecked(state);

    if (state) {
        m_pUi->formatListUnorderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextInsideUnorderedListStateChanged: " << (state ? "enabled" : "disabled"));

    m_pUi->formatListUnorderedPushButton->setChecked(state);

    if (state) {
        m_pUi->formatListOrderedPushButton->setChecked(false);
    }
}

void NoteEditorWidget::onEditorTextInsideTableStateChanged(bool state)
{
    QNTRACE("NoteEditorWidget::onEditorTextInsideTableStateChanged: " << (state ? "enabled" : "disabled"));
    m_pUi->insertTableToolButton->setEnabled(!state);
}

void NoteEditorWidget::onEditorTextFontFamilyChanged(QString fontFamily)
{
    QNTRACE("NoteEditorWidget::onEditorTextFontFamilyChanged: " << fontFamily);

    if (m_lastFontComboBoxFontFamily == fontFamily) {
        QNTRACE("Font family didn't change");
        return;
    }

    m_lastFontComboBoxFontFamily = fontFamily;

    QFont currentFont(fontFamily);
    m_pUi->fontComboBox->setCurrentFont(currentFont);
    QNTRACE("Font family from combo box: " << m_pUi->fontComboBox->currentFont().family()
            << ", font family set by QFont's constructor from it: " << currentFont.family());

    QFontDatabase fontDatabase;
    QList<int> fontSizes = fontDatabase.pointSizes(currentFont.family(), currentFont.styleName());
    // NOTE: it is important to use currentFont.family() in the call above instead of fontFamily variable
    // because the two can be different by presence/absence of apostrophes around the font family name
    if (fontSizes.isEmpty()) {
        QNTRACE("Coulnd't find point sizes for font family " << currentFont.family() << ", will use standard sizes instead");
        fontSizes = fontDatabase.standardSizes();
    }

    m_lastFontSizeComboBoxIndex = 0;    // NOTE: clearing out font sizes combo box causes unwanted update of its index to 0, workarounding it
    m_pUi->fontSizeComboBox->clear();
    int numFontSizes = fontSizes.size();
    QNTRACE("Found " << numFontSizes << " font sizes for font family " << currentFont.family());

    for(int i = 0; i < numFontSizes; ++i) {
        m_pUi->fontSizeComboBox->addItem(QString::number(fontSizes[i]), QVariant(fontSizes[i]));
        QNTRACE("Added item " << fontSizes[i] << "pt for index " << i);
    }

    m_lastFontSizeComboBoxIndex = -1;
}

void NoteEditorWidget::onEditorTextFontSizeChanged(int fontSize)
{
    QNTRACE("NoteEditorWidget::onEditorTextFontSizeChanged: " << fontSize);

    if (m_lastSuggestedFontSize == fontSize) {
        QNTRACE("This font size has already been suggested previously");
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
            QNTRACE("fontSizeComboBox: set current index to " << fontSizeIndex << ", found font size = " << QVariant(fontSize));
        }

        return;
    }

    QNDEBUG("Can't find font size " << fontSize << " within those listed in font size combobox, "
            "will try to choose the closest one instead");

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
            QNWARNING("Can't convert value from font size combo box to int: " << value);
            continue;
        }

        int discrepancy = abs(valueInt - fontSize);
        if (currentSmallestDiscrepancy > discrepancy) {
            currentSmallestDiscrepancy = discrepancy;
            currentClosestIndex = i;
            currentClosestFontSize = valueInt;
            QNTRACE("Updated current closest index to " << i << ": font size = " << valueInt);
        }
    }

    if (currentClosestIndex < 0) {
        QNDEBUG("Couldn't find closest font size to " << fontSize);
        return;
    }

    QNTRACE("Found closest current font size: " << currentClosestFontSize << ", index = " << currentClosestIndex);

    if ( (m_lastFontSizeComboBoxIndex == currentClosestIndex) &&
         (m_lastActualFontSize == currentClosestFontSize) )
    {
        QNTRACE("Neither the font size nor its index within the font combo box have changed");
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
    QNTRACE("NoteEditorWidget::onEditorTextInsertTableDialogRequested");

    QScopedPointer<TableSettingsDialog> tableSettingsDialogHolder(new TableSettingsDialog(this));
    TableSettingsDialog * tableSettingsDialog = tableSettingsDialogHolder.data();
    if (tableSettingsDialog->exec() == QDialog::Rejected) {
        QNTRACE("Returned from TableSettingsDialog::exec: rejected");
        return;
    }

    QNTRACE("Returned from TableSettingsDialog::exec: accepted");
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
    QNDEBUG("NoteEditorWidget::onEditorSpellCheckerNotReady");

    m_pendingEditorSpellChecker = true;
    emit notifyError(QNLocalizedString("Spell checker is loading dictionaries, please wait", this));
}

void NoteEditorWidget::onEditorSpellCheckerReady()
{
    QNDEBUG("NoteEditorWidget::onEditorSpellCheckerReady");

    if (!m_pendingEditorSpellChecker) {
        return;
    }

    m_pendingEditorSpellChecker = false;
    emit notifyError(QNLocalizedString());     // Send the empty message to remove the previous one about the non-ready spell checker
}

void NoteEditorWidget::createConnections(LocalStorageManagerThreadWorker & localStorageWorker)
{
    QNDEBUG("NoteEditorWidget::createConnections");

    // Local signals to localStorageWorker's slots
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,updateNote,Note,bool,bool,QUuid),
                     &localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,findNote,Note,bool,QUuid),
                     &localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,findNotebook,Notebook,QUuid),
                     &localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorWidget,findTag,Tag,QUuid),
                     &localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindTagRequest,Tag,QUuid));

    // localStorageWorker's signals to local slots
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNoteComplete,Note,bool,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNoteFailed,Note,bool,QNLocalizedString,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteEditorWidget,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteEditorWidget,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteEditorWidget,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(NoteEditorWidget,onFindTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteEditorWidget,onExpungeTagComplete,Tag,QUuid));

    // Connect note editor's signals to local slots
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,convertedToNote,Note),
                     this, QNSLOT(NoteEditorWidget,onEditorNoteUpdate,Note));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,cantConvertToNote,QNLocalizedString),
                     this, QNSLOT(NoteEditorWidget,onEditorNoteUpdateFailed,QNLocalizedString));
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,currentNoteChanged,Note),
                     this, QNSLOT(NoteEditorWidget,onEditorNoteUpdate,Note));

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
    QObject::connect(m_pUi->noteEditor, QNSIGNAL(NoteEditor,notifyError,QNLocalizedString),
                     this, QNSIGNAL(NoteEditorWidget,notifyError,QNLocalizedString));

    // Connect toolbar buttons actions to local slots
    QObject::connect(m_pUi->fontBoldPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextBoldToggled));
    QObject::connect(m_pUi->fontItalicPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextItalicToggled));
    QObject::connect(m_pUi->fontUnderlinePushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextUnderlineToggled));
    QObject::connect(m_pUi->fontStrikethroughPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextStrikethroughToggled));
    QObject::connect(m_pUi->formatJustifyLeftPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignLeftAction));
    QObject::connect(m_pUi->formatJustifyCenterPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignCenterAction));
    QObject::connect(m_pUi->formatJustifyRightPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAlignRightAction));
    QObject::connect(m_pUi->insertHorizontalLinePushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextAddHorizontalLineAction));
    QObject::connect(m_pUi->formatIndentMorePushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextIncreaseIndentationAction));
    QObject::connect(m_pUi->formatIndentLessPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextDecreaseIndentationAction));
    QObject::connect(m_pUi->formatListUnorderedPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsertUnorderedListAction));
    QObject::connect(m_pUi->formatListOrderedPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorTextInsertOrderedListAction));
    QObject::connect(m_pUi->chooseTextColorToolButton, QNSIGNAL(ColorPickerToolButton,colorSelected,QColor),
                     this, QNSLOT(NoteEditorWidget,onEditorChooseTextColor,QColor));
    QObject::connect(m_pUi->chooseBackgroundColorToolButton, QNSIGNAL(ColorPickerToolButton,colorSelected,QColor),
                     this, QNSLOT(NoteEditorWidget,onEditorChooseBackgroundColor,QColor));
    QObject::connect(m_pUi->spellCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(NoteEditorWidget,onEditorSpellCheckStateChanged,int));
    QObject::connect(m_pUi->insertToDoCheckboxPushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteEditorWidget,onEditorInsertToDoCheckBoxAction));
    QObject::connect(m_pUi->insertTableToolButton, QNSIGNAL(InsertTableToolButton,createdTable,int,int,double,bool),
                     this, QNSLOT(NoteEditorWidget,onEditorInsertTable,int,int,double,bool));
}

} // namespace quentier

#include "NoteEditorTabWidgetManager.h"
#include "models/TagModel.h"
#include "widgets/NoteEditorWidget.h"
#include <quentier/types/Note.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QUndoStack>

#define DEFAULT_MAX_NUM_NOTES (5)
#define BLANK_NOTE_KEY QStringLiteral("BlankNoteId")

namespace quentier {

NoteEditorTabWidgetManager::NoteEditorTabWidgetManager(const Account & account, LocalStorageManagerThreadWorker & localStorageWorker,
                                                       NoteCache & noteCache, NotebookCache & notebookCache,
                                                       TagCache & tagCache, TagModel & tagModel,
                                                       QTabWidget * tabWidget, QObject * parent) :
    QObject(parent),
    m_currentAccount(account),
    m_localStorageWorker(localStorageWorker),
    m_noteCache(noteCache),
    m_notebookCache(notebookCache),
    m_tagCache(tagCache),
    m_tagModel(tagModel),
    m_pTabWidget(tabWidget),
    m_pBlankNoteEditor(Q_NULLPTR),
    m_blankNoteTab(-1),
    m_pBlankNoteTabUndoStack(new QUndoStack(this)),
    m_maxNumNotes(-1)
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(QStringLiteral("NoteEditor"));
    QVariant maxNumNoteTabsData = appSettings.value(QStringLiteral("MaxNumNoteTabs"));
    appSettings.endGroup();

    bool conversionResult = false;
    int maxNumNoteTabs = maxNumNoteTabsData.toInt(&conversionResult);
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("NoteEditorTabWidgetManager: no persisted max num note tabs setting, "
                               "fallback to the default value of ") << DEFAULT_MAX_NUM_NOTES);
        m_maxNumNotes = DEFAULT_MAX_NUM_NOTES;
    }
    else {
        QNDEBUG(QStringLiteral("NoteEditorTabWidgetManager: max num note tabs: ") << maxNumNoteTabs);
        m_maxNumNotes = maxNumNoteTabs;
    }

    m_pBlankNoteEditor = new NoteEditorWidget(m_currentAccount, m_localStorageWorker,
                                              m_noteCache, m_notebookCache, m_tagCache,
                                              m_tagModel, m_pBlankNoteTabUndoStack);
    m_blankNoteTab = m_pTabWidget->addTab(m_pBlankNoteEditor, BLANK_NOTE_KEY);
}

void NoteEditorTabWidgetManager::setMaxNumNotes(const int maxNumNotes)
{
    QNDEBUG(QStringLiteral("NoteEditorTabWidgetManager::setMaxNumNotes: ") << maxNumNotes);

    if (m_maxNumNotes == maxNumNotes) {
        QNDEBUG(QStringLiteral("Max number of notes hasn't changed"));
        return;
    }

    if (m_maxNumNotes < maxNumNotes) {
        m_maxNumNotes = maxNumNotes;
        QNDEBUG(QStringLiteral("Max number of notes has been increased to ") << maxNumNotes);
        return;
    }

    int currentNumNotes = numNotes();

    m_maxNumNotes = maxNumNotes;
    QNDEBUG(QStringLiteral("Max number of notes has been decreased to ") << maxNumNotes);

    if (currentNumNotes <= maxNumNotes) {
        return;
    }

    int numNotesToShrink = currentNumNotes - maxNumNotes;
    // TODO: shrink this amount of notes
    Q_UNUSED(numNotesToShrink)
}

int NoteEditorTabWidgetManager::numNotes() const
{
    if (Q_UNLIKELY(!m_pTabWidget)) {
        return 0;
    }

    if (m_pBlankNoteEditor) {
        return std::max(m_pTabWidget->count() - 1, 0);
    }
    else {
        return m_pTabWidget->count();
    }
}

void NoteEditorTabWidgetManager::addNote(const Note & note)
{
    QNDEBUG(QStringLiteral("NoteEditorTabWidgetManager::addNote: ") << note);

    // TODO: implement
}

} // namespace quentier

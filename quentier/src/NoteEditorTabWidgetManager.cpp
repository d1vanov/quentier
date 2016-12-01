#include "NoteEditorTabWidgetManager.h"
#include "models/TagModel.h"
#include "widgets/NoteEditorWidget.h"
#include "widgets/TabWidget.h"
#include <quentier/types/Note.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QUndoStack>
#include <QTabBar>
#include <algorithm>

#define DEFAULT_MAX_NUM_NOTES (5)
#define BLANK_NOTE_KEY QStringLiteral("BlankNoteId")

namespace quentier {

NoteEditorTabWidgetManager::NoteEditorTabWidgetManager(const Account & account, LocalStorageManagerThreadWorker & localStorageWorker,
                                                       NoteCache & noteCache, NotebookCache & notebookCache,
                                                       TagCache & tagCache, TagModel & tagModel,
                                                       TabWidget * tabWidget, QObject * parent) :
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
    m_maxNumNotes(-1),
    m_shownNoteLocalUids()
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

    m_pTabWidget->tabBar()->hide();
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

    m_shownNoteLocalUids.set_capacity(static_cast<size_t>(std::max(maxNumNotes, 0)));
    checkAndCloseOlderNoteEditors();
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

void NoteEditorTabWidgetManager::addNote(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabWidgetManager::addNote: ") << noteLocalUid);

    // TODO: implement

    m_shownNoteLocalUids.push_back(noteLocalUid);

    int currentNumNotes = numNotes();
    if (currentNumNotes <= m_maxNumNotes) {
        QNDEBUG(QStringLiteral("The addition of note ") << noteLocalUid
                << QStringLiteral(" doesn't cause the overflow of max allowed number of note editor tabs"));
        return;
    }

    checkAndCloseOlderNoteEditors();
}

void NoteEditorTabWidgetManager::checkAndCloseOlderNoteEditors()
{
    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        if (Q_UNLIKELY(i == m_blankNoteTab)) {
            continue;
        }

        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        const QString & noteLocalUid = pNoteEditorWidget->noteLocalUid();
        auto it = std::find(m_shownNoteLocalUids.begin(), m_shownNoteLocalUids.end(), noteLocalUid);
        if (it == m_shownNoteLocalUids.end()) {
            // TODO: close note editor widget - if it has a modified unsaved note, need to save is synchronously
            m_pTabWidget->removeTab(i);
        }
    }
}

} // namespace quentier

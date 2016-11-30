/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_MAINWINDOW_H
#define QUENTIER_MAINWINDOW_H

#include "AccountManager.h"
#include "models/NotebookCache.h"
#include "models/TagCache.h"
#include "models/SavedSearchCache.h"
#include "models/NoteCache.h"
#include "models/NotebookModel.h"
#include "models/TagModel.h"
#include "models/SavedSearchModel.h"
#include "models/NoteModel.h"
#include "models/FavoritesModel.h"
#include "widgets/NoteEditorWidget.h"
#include <quentier/utility/ShortcutManager.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/synchronization/SynchronizationManager.h>

#include <QtCore>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QMainWindow>
#else
#include <QMainWindow>
#endif

#include <QTextListFormat>
#include <QMap>
#include <QStandardItemModel>

namespace Ui {
class MainWindow;
}

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QUndoStack)
QT_FORWARD_DECLARE_CLASS(QActionGroup)

QT_FORWARD_DECLARE_CLASS(ColumnChangeRerouter)

namespace quentier {
QT_FORWARD_DECLARE_CLASS(NoteEditor)
}

using namespace quentier;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget * pParentWidget = Q_NULLPTR);
    virtual ~MainWindow();

public Q_SLOTS:
    void onSetStatusBarText(QString message, const int duration = 0);

Q_SIGNALS:
    // private signals
    void localStorageSwitchUserRequest(Account account, bool startFromScratch, QUuid requestId);
    void authenticate();

private Q_SLOTS:
    void onUndoAction();
    void onRedoAction();
    void onCopyAction();
    void onCutAction();
    void onPasteAction();

    void onNoteTextSelectAllToggled();
    void onNoteTextBoldToggled();
    void onNoteTextItalicToggled();
    void onNoteTextUnderlineToggled();
    void onNoteTextStrikethroughToggled();
    void onNoteTextAlignLeftAction();
    void onNoteTextAlignCenterAction();
    void onNoteTextAlignRightAction();
    void onNoteTextAddHorizontalLineAction();
    void onNoteTextIncreaseFontSizeAction();
    void onNoteTextDecreaseFontSizeAction();
    void onNoteTextHighlightAction();
    void onNoteTextIncreaseIndentationAction();
    void onNoteTextDecreaseIndentationAction();
    void onNoteTextInsertUnorderedListAction();
    void onNoteTextInsertOrderedListAction();
    void onNoteTextInsertTableDialogAction();
    void onNoteTextEditHyperlinkAction();
    void onNoteTextCopyHyperlinkAction();
    void onNoteTextRemoveHyperlinkAction();
    void onNoteTextSpellCheckToggled();
    void onShowNoteSource();

    void onFindInsideNoteAction();
    void onFindPreviousInsideNoteAction();
    void onReplaceInsideNoteAction();

    // Synchronization manager slots
    void onSynchronizationManagerFailure(QNLocalizedString errorDescription);
    void onSynchronizationFinished(Account account);
    void onAuthenticationFinished(bool success, QNLocalizedString errorDescription,
                                  Account account);
    void onAuthenticationRevoked(bool success, QNLocalizedString errorDescription,
                                 qevercloud::UserID userId);
    void onRateLimitExceeded(qint32 secondsToWait);
    void onRemoteToLocalSyncDone();
    void onSynchronizationProgressUpdate(QNLocalizedString message, double workDonePercentage);
    void onRemoteToLocalSyncStopped();
    void onSendLocalChangesStopped();

    // AccountManager slots
    void onEvernoteAccountAuthenticationRequested(QString host);
    void onAccountSwitched(Account account);
    void onAccountManagerError(QNLocalizedString errorDescription);

    // Look and feel settings slots
    void onSwitchIconsToNativeAction();
    void onSwitchIconsToTangoAction();
    void onSwitchIconsToOxygenAction();

    void onSwitchPanelStyleToBuiltIn();
    void onSwitchPanelStyleToLighter();
    void onSwitchPanelStyleToDarker();

    // Test notes for debugging
    void onSetTestNoteWithEncryptedData();
    void onSetTestNoteWithResources();
    void onSetTestReadOnlyNote();
    void onSetInkNote();

    void onNoteEditorError(QNLocalizedString error);

    void onNoteEditorSpellCheckerNotReady();
    void onNoteEditorSpellCheckerReady();

    void onAddAccountActionTriggered(bool checked);
    void onManageAccountsActionTriggered(bool checked);
    void onSwitchAccountActionToggled(bool checked);

    void onLocalStorageSwitchUserRequestComplete(Account account, QUuid requestId);
    void onLocalStorageSwitchUserRequestFailed(Account account, QNLocalizedString errorDescription, QUuid requestId);

private:
    void setupThemeIcons();

    void setupAccountManager();

    void setupLocalStorageManager();

    void setupModels();
    void clearModels();

    void setupViews();
    void clearViews();

    void setupSynchronizationManager();
    void clearSynchronizationManager();

    void setupDefaultShortcuts();
    void setupUserShortcuts();

    void setupConsumerKeyAndSecret(QString & consumerKey, QString & consumerSecret);

    void connectActionsToSlots();
    void addMenuActionsToMainWindow();

    NoteEditorWidget * currentNoteEditor();

    void connectSynchronizationManager();
    void disconnectSynchronizationManager();
    void onSyncStopped();

    void prepareTestNoteWithResources();
    void prepareTestInkNote();

    void persistChosenIconTheme(const QString & iconThemeName);
    void refreshChildWidgetsThemeIcons();

    template <class T>
    void refreshThemeIcons();

    class StyleSheetProperty
    {
    public:
        struct Target
        {
            enum type {
                None = 0,
                ButtonHover,
                ButtonPressed
            };
        };

        StyleSheetProperty(const Target::type targetType = Target::None,
                           const char * name = Q_NULLPTR,
                           const QString & value = QString()) :
            m_targetType(targetType),
            m_name(name),
            m_value(value)
        {}

        Target::type    m_targetType;
        const char *    m_name;
        QString         m_value;
    };

    typedef QVector<StyleSheetProperty> StyleSheetProperties;

    void collectBaseStyleSheets();
    void setupPanelOverlayStyleSheets();
    void getPanelStyleSheetProperties(const QString & panelStyleOption, StyleSheetProperties & properties) const;
    void setPanelsOverlayStyleSheet(const StyleSheetProperties & properties);

    // This method performs a nasty hack - it searches for some properties within
    // the passed in stylesheet and alters some of these; the whole workflow is based
    // on weak assumptions about the structure of the stylesheet so once it is sufficiently
    // altered, this method would stop work. Don't program like this, kids.
    QString alterStyleSheet(const QString & originalStyleSheet,
                            const StyleSheetProperties & properties);

    bool isInsideStyleBlock(const QString & styleSheet, const QString & styleBlockStartSearchString,
                            const int currentIndex, bool & error) const;

private:
    Ui::MainWindow *        m_pUI;
    QWidget *               m_currentStatusBarChildWidget;
    QString                 m_lastNoteEditorHtml;

    QString                 m_nativeIconThemeName;

    QActionGroup *          m_availableAccountsActionGroup;

    AccountManager *            m_pAccountManager;
    QScopedPointer<Account>     m_pAccount;

    QThread *                   m_pLocalStorageManagerThread;
    LocalStorageManagerThreadWorker *   m_pLocalStorageManager;

    QUuid                       m_lastLocalStorageSwitchUserRequest;

    QThread *                   m_pSynchronizationManagerThread;
    SynchronizationManager *    m_pSynchronizationManager;
    QString                     m_synchronizationManagerHost;

    NotebookCache           m_notebookCache;
    TagCache                m_tagCache;
    SavedSearchCache        m_savedSearchCache;
    NoteCache               m_noteCache;

    NotebookModel *         m_pNotebookModel;
    TagModel *              m_pTagModel;
    SavedSearchModel *      m_pSavedSearchModel;
    NoteModel *             m_pNoteModel;

    ColumnChangeRerouter *  m_pNotebookModelColumnChangeRerouter;
    ColumnChangeRerouter *  m_pTagModelColumnChangeRerouter;

    NoteModel *             m_pDeletedNotesModel;
    FavoritesModel *        m_pFavoritesModel;

    QStandardItemModel      m_blankModel;

    Notebook                m_testNotebook;
    Note                    m_testNote;

    QUndoStack *            m_pUndoStack;

    struct StyleSheetInfo
    {
        QPointer<QWidget>   m_targetWidget;
        QString             m_baseStyleSheet;
        QString             m_overlayStyleSheet;
    };

    QHash<QWidget*, StyleSheetInfo>    m_styleSheetInfo;

    QString                 m_currentPanelStyle;

    quentier::ShortcutManager   m_shortcutManager;
};

#endif // QUENTIER_MAINWINDOW_H

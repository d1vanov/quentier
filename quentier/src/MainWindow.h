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

#include <quentier/types/Notebook.h>
#include <quentier/types/Note.h>
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/utility/ShortcutManager.h>

#include <QtCore>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QMainWindow>
#else
#include <QMainWindow>
#endif
#include <QTextListFormat>

namespace Ui {
class MainWindow;
}

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace quentier {
QT_FORWARD_DECLARE_CLASS(NoteEditor)
}

using quentier::QNLocalizedString;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget * pParentWidget = Q_NULLPTR);
    virtual ~MainWindow();

public Q_SLOTS:
    void onSetStatusBarText(QString message, const int duration = 0);

private Q_SLOTS:
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
    void onNoteTextEditHyperlinkAction();
    void onNoteTextCopyHyperlinkAction();
    void onNoteTextRemoveHyperlinkAction();

    void onNoteChooseTextColor(QColor color);
    void onNoteChooseBackgroundColor(QColor color);

    void onNoteTextSpellCheckToggled();
    void onNoteTextInsertToDoCheckBoxAction();

    void onNoteTextInsertTableDialogAction();
    void onNoteTextInsertTable(int rows, int columns, double width, bool relativeWidth);

    void onShowNoteSource();
    void onSetTestNoteWithEncryptedData();
    void onSetTestNoteWithResources();
    void onSetTestReadOnlyNote();

    void onFindInsideNoteAction();
    void onFindPreviousInsideNoteAction();
    void onReplaceInsideNoteAction();

    void onFindAndReplaceWidgetClosed();
    void onTextToFindInsideNoteEdited(const QString & textToFind);
    void onFindNextInsideNote(const QString & textToFind, const bool matchCase);
    void onFindPreviousInsideNote(const QString & textToFind, const bool matchCase);
    void onFindInsideNoteCaseSensitivityChanged(const bool matchCase);
    void onReplaceInsideNote(const QString & textToReplace, const QString & replacementText, const bool matchCase);
    void onReplaceAllInsideNote(const QString & textToReplace, const QString & replacementText, const bool matchCase);

    void onNoteEditorHtmlUpdate(QString html);
    void onNoteEditorError(QNLocalizedString error);

    void onNoteEditorSpellCheckerNotReady();
    void onNoteEditorSpellCheckerReady();

    // Slots used to reflect the change of formatting for the piece of text being the one currently pointed to by the text cursor in the note editor
    void onNoteEditorBoldStateChanged(bool state);
    void onNoteEditorItalicStateChanged(bool state);
    void onNoteEditorUnderlineStateChanged(bool state);
    void onNoteEditorStrikethroughStateChanged(bool state);
    void onNoteEditorAlignLeftStateChanged(bool state);
    void onNoteEditorAlignCenterStateChanged(bool state);
    void onNoteEditorAlignRightStateChanged(bool state);
    void onNoteEditorInsideOrderedListStateChanged(bool state);
    void onNoteEditorInsideUnorderedListStateChanged(bool state);
    void onNoteEditorInsideTableStateChanged(bool state);
    void onNoteEditorFontFamilyChanged(QString fontFamily);
    void onNoteEditorFontSizeChanged(int fontSize);

    void onFontComboBoxFontChanged(QFont font);
    void onFontSizeComboBoxIndexChanged(int currentIndex);

private:
    void checkThemeIconsAndSetFallbacks();
    void updateNoteHtmlView(QString html);

    void setupDefaultShortcuts();
    void setupUserShortcuts();

    void setupConsumerKeyAndSecret(QString & consumerKey, QString & consumerSecret);

    void connectActionsToSlots();
    void connectActionsToEditorSlots();
    void connectEditorSignalsToSlots();
    void addMenuActionsToMainWindow();

    void prepareTestNoteWithResources();

private:
    Ui::MainWindow * m_pUI;
    QWidget * m_currentStatusBarChildWidget;
    quentier::NoteEditor * m_pNoteEditor;
    QString m_lastNoteEditorHtml;

    quentier::Notebook    m_testNotebook;
    quentier::Note        m_testNote;

    int                    m_lastFontSizeComboBoxIndex;
    QString                m_lastFontComboBoxFontFamily;

    QUndoStack *           m_pUndoStack;

    quentier::ShortcutManager  m_shortcutManager;
};

#endif // QUENTIER_MAINWINDOW_H

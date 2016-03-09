#ifndef __QUTE_NOTE__MAINWINDOW_H
#define __QUTE_NOTE__MAINWINDOW_H

#include <qute_note/types/Notebook.h>
#include <qute_note/types/Note.h>
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/ShortcutManager.h>

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

namespace qute_note {
QT_FORWARD_DECLARE_CLASS(NoteEditor)
}

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
    void onNoteEditorError(QString error);

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

    bool consumerKeyAndSecret(QString & consumerKey, QString & consumerSecret, QString & error);

    void connectActionsToSlots();
    void connectActionsToEditorSlots();
    void connectEditorSignalsToSlots();
    void addMenuActionsToMainWindow();

private:
    Ui::MainWindow * m_pUI;
    QWidget * m_currentStatusBarChildWidget;
    qute_note::NoteEditor * m_pNoteEditor;
    QString m_lastNoteEditorHtml;

    qute_note::Notebook    m_testNotebook;
    qute_note::Note        m_testNote;

    int                    m_lastFontSizeComboBoxIndex;
    QString                m_lastFontComboBoxFontFamily;

    QUndoStack *           m_pUndoStack;

    qute_note::ShortcutManager  m_shortcutManager;
};

#endif // __QUTE_NOTE__MAINWINDOW_H

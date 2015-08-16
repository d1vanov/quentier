#ifndef __QUTE_NOTE__MAINWINDOW_H
#define __QUTE_NOTE__MAINWINDOW_H

#include <qute_note/types/Notebook.h>
#include <qute_note/types/Note.h>

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

namespace qute_note {
QT_FORWARD_DECLARE_CLASS(NoteEditor)
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget * pParentWidget = nullptr);
    virtual ~MainWindow();

private:
    void connectActionsToEditorSlots();

public Q_SLOTS:
    void onSetStatusBarText(QString message, const int duration = 0);

private Q_SLOTS:
    void noteTextBold();
    void noteTextItalic();
    void noteTextUnderline();
    void noteTextStrikeThrough();
    void noteTextAlignLeft();
    void noteTextAlignCenter();
    void noteTextAlignRight();
    void noteTextAddHorizontalLine();
    void noteTextIncreaseIndentation();
    void noteTextDecreaseIndentation();
    void noteTextInsertUnorderedList();
    void noteTextInsertOrderedList();

    void noteChooseTextColor();
    void noteChooseSelectedTextColor();

    void noteTextSpellCheck() { /* TODO: implement */ }
    void noteTextInsertToDoCheckBox();

    void noteTextInsertTableDialog();
    void noteTextInsertTable(int rows, int columns, double width, bool relativeWidth);

    void onShowNoteSource();
    void onSetTestNoteWithEncryptedData();

    void onNoteEditorHtmlUpdate(QString html);

private:
    void checkThemeIconsAndSetFallbacks();
    void updateNoteHtmlView(QString html);

    bool consumerKeyAndSecret(QString & consumerKey, QString & consumerSecret, QString & error);

private:
    Ui::MainWindow * m_pUI;
    QWidget * m_currentStatusBarChildWidget;
    qute_note::NoteEditor * m_pNoteEditor;
    QString m_lastNoteEditorHtml;

    qute_note::Notebook    m_testNotebook;
    qute_note::Note        m_testNote;
};

#endif // __QUTE_NOTE__MAINWINDOW_H

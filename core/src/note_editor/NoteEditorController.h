#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_CONTROLLER_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_CONTROLLER_H

#include <QObject>
#include <QMimeType>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QWebView)
QT_FORWARD_DECLARE_CLASS(QFont)
QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QMimeData)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)

class NoteEditorController : public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorController(QWebView * webView, QObject * parent = nullptr);
    virtual ~NoteEditorController();

    bool setNote(const Note & note, QString & errorDescription);
    bool getNote(Note & note, QString & errorDescription) const;

    const std::pair<QByteArray, QMimeType> * findInsertedResource(const QString & hash);

    bool modified() const;

Q_SIGNALS:
    void contentChanged();
    void notifyError(QString error);

public Q_SLOTS:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikethrough();
    void alignLeft();
    void alignCenter();
    void alignRight();
    void insertToDoCheckbox();
    void setSpellcheck(const bool enabled);
    void setFont(const QFont & font);
    void setFontHeight(const int height);
    void setFontColor(const QColor & color);
    void setBackgroundColor(const QColor & color);
    void insertHorizontalLine();
    void changeIndentation(const bool increase);
    void insertBulletedList();
    void insertNumberedList();
    void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels);
    void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth);

    void insertNewResource(QByteArray data, QMimeType mimeType);

    void onNoteLoadCancelled();

private:
    void execJavascriptCommand(const QString & command);
    void execJavascriptCommand(const QString & command, const QString & args);

private:
    QWebView *      m_pWebView;
    bool            m_modified;

    QHash<QString, std::pair<QByteArray, QMimeType> >  m_insertedResourcesByHash;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_CONTROLLER_H

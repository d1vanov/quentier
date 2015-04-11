#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_H

#include <tools/qt4helper.h>
#include <QWebView>
#include <QString>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QByteArray)
QT_FORWARD_DECLARE_CLASS(QMimeType)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(INoteEditorResourceInserter)

class NoteEditor: public QWebView
{
    Q_OBJECT
public:

public:
    explicit NoteEditor(QWidget * parent = nullptr);

    void setNote(const Note & note);
    const Note * getNote();

    bool isModified() const;

public:
    // ====== Interface for various resource inserters by mime type =========
    void addResourceInserterForMimeType(const QString & mimeTypeName,
                                        INoteEditorResourceInserter * pResourceInserter);

    // returns true if resource inserter was found by mime type and removed
    bool removeResourceInserterForMimeType(const QString & mimeTypeName);

    bool hasResourceInsertedForMimeType(const QString & mimeTypeName) const;

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

    void onNoteLoadCancelled();

private:
    void execJavascriptCommand(const QString & command);
    void execJavascriptCommand(const QString & command, const QString & args);

    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;
    void dropFile(QString & filepath);
    void attachResourceToNote(const QByteArray & data, const QString & dataHash, const QMimeType & mimeType);

    template <typename T>
    QString composeHtmlTable(const T width, const T singleColumnWidth, const int rows, const int columns,
                             const bool relative);

    void insertImage(const QByteArray & data,  const QString & dataHash, const QMimeType & mimeType);

private:
    Note *  m_pNote;
    bool    m_modified;
    QHash<QString, INoteEditorResourceInserter*>    m_noteEditorResourceInserters;
};

} // namespace qute_note

#endif //__QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_H

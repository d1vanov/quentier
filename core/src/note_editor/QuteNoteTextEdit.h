#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__QUTE_NOTE_TEXT_EDIT_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__QUTE_NOTE_TEXT_EDIT_H

#include <tools/Linkage.h>
#include <tools/qt4helper.h>
#include <client/enml/ENMLConverter.h>
#include <QTextEdit>

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QImage)

class QUTE_NOTE_EXPORT QuteNoteTextEdit: public QTextEdit
{
    Q_OBJECT
public:
    explicit QuteNoteTextEdit(QWidget * parent = nullptr);
    virtual ~QuteNoteTextEdit();

    enum ECustomTextObjectTextFormat {
        TODO_CHKBOX_TXT_FMT_UNCHECKED = QTextFormat::UserObject + 1,
        TODO_CHKBOX_TXT_FMT_CHECKED   = QTextFormat::UserObject + 2,
        MEDIA_RESOURCE_TXT_FORMAT     = QTextFormat::UserObject + 3
    };

    enum ECustomTextObjectTextProperties {
        TODO_CHKBOX_TXT_DATA_UNCHECKED = 1,
        TODO_CHKBOX_TXT_DATA_CHECKED   = 2,
        MEDIA_RESOURCE_TXT_DATA        = 3
    };

    virtual bool canInsertFromMimeData(const QMimeData * source) const Q_DECL_OVERRIDE;
    virtual void insertFromMimeData(const QMimeData * source) Q_DECL_OVERRIDE;

    void changeIndentation(const bool increase);
    void mergeFormatOnWordOrSelection(const QTextCharFormat & format);

    bool noteRichTextToENML(QString & ENML, QString & errorDescription) const;

    void insertCheckedToDoCheckboxAtCursor(QTextCursor cursor);
    void insertUncheckedToDoCheckboxAtCursor(QTextCursor cursor);

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void dropImage(const QUrl & url, const QImage & image);
    void insertToDoCheckbox(QTextCursor cursor, const bool checked);

private:
    mutable size_t    m_droppedImageCounter;
    qute_note::ENMLConverter  m_converter;
};

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__QUTE_NOTE_TEXT_EDIT_H

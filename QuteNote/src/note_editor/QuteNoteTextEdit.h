#ifndef __QUTE_NOTE___QUTE_NOTE_TEXT_EDIT_H
#define __QUTE_NOTE___QUTE_NOTE_TEXT_EDIT_H

#include <QTextEdit>

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QImage)

class QuteNoteTextEdit: public QTextEdit
{
    Q_OBJECT
public:
    explicit QuteNoteTextEdit(QWidget * parent = nullptr);
    virtual ~QuteNoteTextEdit() final override {}

public:
    enum ECheckboxTextFormat {
        TODO_CHKBOX_TXT_FMT_UNCHECKED = QTextFormat::UserObject + 1,
        TODO_CHKBOX_TXT_FMT_CHECKED   = QTextFormat::UserObject + 2
    };

    enum ECheckboxTextProperties {
        TODO_CHKBOX_TXT_DATA_UNCHECKED = 1,
        TODO_CHKBOX_TXT_DATA_CHECKED   = 2
    };

public:
    virtual bool canInsertFromMimeData(const QMimeData * source) const final override;
    virtual void insertFromMimeData(const QMimeData * source) final override;
    void changeIndentation(const bool increase);
    void mergeFormatOnWordOrSelection(const QTextCharFormat & format);

    void noteRichTextToENML(QString & ENML) const;

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) final override;
    virtual void mousePressEvent(QMouseEvent * pEvent) final override;
    virtual void mouseMoveEvent(QMouseEvent * pEvent) final override;

private:
    void dropImage(const QUrl & url, const QImage & image);

private:
    mutable size_t m_droppedImageCounter;
};

#endif // __QUTE_NOTE___QUTE_NOTE_TEXT_EDIT_H

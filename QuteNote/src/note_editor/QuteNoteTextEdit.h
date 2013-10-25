#ifndef __QUTE_NOTE___QUTE_NOTE_TEXT_EDIT_H
#define __QUTE_NOTE___QUTE_NOTE_TEXT_EDIT_H

#include <QTextEdit>

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QImage)

namespace qute_note {
QT_FORWARD_DECLARE_CLASS(Note)
}

class QuteNoteTextEdit: public QTextEdit
{
    Q_OBJECT
public:
    explicit QuteNoteTextEdit(QWidget * parent = nullptr);
    virtual ~QuteNoteTextEdit() final override {}

public:
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

public:
    virtual bool canInsertFromMimeData(const QMimeData * source) const final override;
    virtual void insertFromMimeData(const QMimeData * source) final override;
    void changeIndentation(const bool increase);
    void mergeFormatOnWordOrSelection(const QTextCharFormat & format);

    const qute_note::Note * getNotePtr() const;
    void setNote(const qute_note::Note & note);

    void noteRichTextToENML(QString & ENML) const;

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) final override;
    virtual void mousePressEvent(QMouseEvent * pEvent) final override;
    virtual void mouseMoveEvent(QMouseEvent * pEvent) final override;

private:
    void dropImage(const QUrl & url, const QImage & image);

private:
    mutable size_t    m_droppedImageCounter;
    const qute_note::Note * m_pNote;
};

#endif // __QUTE_NOTE___QUTE_NOTE_TEXT_EDIT_H

#ifndef TEXT_EDIT_WITH_CHECKBOXES_H
#define TEXT_EDIT_WITH_CHECKBOXES_H

#include <QTextEdit>

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QImage)

class TextEditWithCheckboxes: public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEditWithCheckboxes(QWidget * parent = nullptr);
    virtual ~TextEditWithCheckboxes() override {}

public:
    bool canInsertFromMimeData(const QMimeData * source) const;
    void insertFromMimeData(const QMimeData * source);

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) override;
    virtual void mousePressEvent(QMouseEvent * pEvent) override;
    virtual void mouseMoveEvent(QMouseEvent * pEvent) override;

private:
    void dropImage(const QUrl & url, const QImage & image);

private:
    mutable size_t m_droppedImageCounter;
};

#endif // TEXT_EDIT_WITH_CHECKBOXES_H

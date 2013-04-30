#ifndef TODO_CHECK_BOX_TEXT_OBJECT_H
#define TODO_CHECK_BOX_TEXT_OBJECT_H

#include <QTextObjectInterface>
#include <QCheckBox>

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QTextFormat)
QT_FORWARD_DECLARE_CLASS(QPainter)
QT_FORWARD_DECLARE_CLASS(QRectF)
QT_FORWARD_DECLARE_CLASS(QSizeF)

class ToDoCheckboxTextObject: public QCheckBox, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    ToDoCheckboxTextObject();
    virtual ~ToDoCheckboxTextObject() {}

public:
    enum { CheckboxTextFormat = QTextFormat::UserObject + 1 };
    enum CheckboxTextProperties { CheckboxTextData = 1 };

public:
    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format);
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format);

public:
    // Get QImage from QIcon
    QImage getImage() const;
};

#endif // TODO_CHECK_BOX_TEXT_OBJECT_H

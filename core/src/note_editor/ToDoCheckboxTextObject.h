#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__TODO_CHECKBOX_TEXT_OBJECT_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__TODO_CHECKBOX_TEXT_OBJECT_H

#include <tools/Linkage.h>
#include <QTextObjectInterface>

// NOTE: I have to declare two classes instead of a template one as Q_OBJECT does not support template classes

class QUTE_NOTE_EXPORT ToDoCheckboxTextObjectUnchecked: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ToDoCheckboxTextObjectUnchecked() {}
    virtual ~ToDoCheckboxTextObjectUnchecked() final override {}

public:
    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) final override;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) final override;
};

class QUTE_NOTE_EXPORT ToDoCheckboxTextObjectChecked: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ToDoCheckboxTextObjectChecked() {}
    virtual ~ToDoCheckboxTextObjectChecked() final override {}

public:
    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) final override;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) final override;
};

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__TODO_CHECKBOX_TEXT_OBJECT_H

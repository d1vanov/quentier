#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__TODO_CHECKBOX_TEXT_OBJECT_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__TODO_CHECKBOX_TEXT_OBJECT_H

#include <tools/Linkage.h>
#include <tools/qt4helper.h>
#include <QTextObjectInterface>
#include <QCheckBox>

// NOTE: I have to declare two classes instead of a template one as Q_OBJECT does not support template classes

class QUTE_NOTE_EXPORT ToDoCheckboxTextObjectUnchecked: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ToDoCheckboxTextObjectUnchecked();
    virtual ~ToDoCheckboxTextObjectUnchecked() {}

public:
    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) Q_DECL_OVERRIDE;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) Q_DECL_OVERRIDE;

private:
    QCheckBox   m_checkbox;
};

class QUTE_NOTE_EXPORT ToDoCheckboxTextObjectChecked: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ToDoCheckboxTextObjectChecked();
    virtual ~ToDoCheckboxTextObjectChecked() {}

public:
    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) Q_DECL_OVERRIDE;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) Q_DECL_OVERRIDE;

private:
    QCheckBox   m_checkbox;
};

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__TODO_CHECKBOX_TEXT_OBJECT_H

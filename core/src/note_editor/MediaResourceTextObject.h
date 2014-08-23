#ifndef MEDIARESOURCETEXTOBJECT_H
#define MEDIARESOURCETEXTOBJECT_H

#include <tools/Linkage.h>
#include <QTextObjectInterface>

class QUTE_NOTE_EXPORT MediaResourceTextObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit MediaResourceTextObject() {}
    virtual ~MediaResourceTextObject() override {}
    
public:
    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) = 0;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) = 0;
};

#endif // MEDIARESOURCETEXTOBJECT_H

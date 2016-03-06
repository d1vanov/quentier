#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

class ResizableImageJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    ResizableImageJavaScriptHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void imageResourceResized();

public Q_SLOTS:
    void notifyImageResourceResized();
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H

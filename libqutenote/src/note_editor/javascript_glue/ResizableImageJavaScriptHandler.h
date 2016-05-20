#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

class ResizableImageJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    ResizableImageJavaScriptHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void imageResourceResized(bool pushUndoCommand);

public Q_SLOTS:
    void notifyImageResourceResized(bool pushUndoCommand);
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H

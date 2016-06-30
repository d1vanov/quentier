#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_RESIZABLE_IMAGE_JAVASCRIPT_HANDLER_H

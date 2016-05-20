#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_TABLE_RESIZE_JAVASCRIPT_HANDLER_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_TABLE_RESIZE_JAVASCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

class TableResizeJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit TableResizeJavaScriptHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void tableResized();

public Q_SLOTS:
    void onTableResize();
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_JAVASCRIPT_GLUE_TABLE_RESIZE_JAVASCRIPT_HANDLER_H

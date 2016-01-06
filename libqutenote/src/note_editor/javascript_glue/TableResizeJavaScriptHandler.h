#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TABLE_RESIZE_JAVASCRIPT_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TABLE_RESIZE_JAVASCRIPT_HANDLER_H

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

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TABLE_RESIZE_JAVASCRIPT_HANDLER_H

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TABLE_RESIZE_JAVASCRIPT_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TABLE_RESIZE_JAVASCRIPT_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TABLE_RESIZE_JAVASCRIPT_HANDLER_H

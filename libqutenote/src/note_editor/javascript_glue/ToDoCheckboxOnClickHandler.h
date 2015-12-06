#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TO_DO_CHECKBOX_ON_CLICK_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TO_DO_CHECKBOX_ON_CLICK_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

class ToDoCheckboxOnClickHandler: public QObject
{
    Q_OBJECT
public:
    explicit ToDoCheckboxOnClickHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void toDoCheckboxClicked(quint64 enToDoCheckboxId);
    void notifyError(QString error);

public Q_SLOTS:
    void onToDoCheckboxClicked(QString enToDoCheckboxId);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TO_DO_CHECKBOX_ON_CLICK_HANDLER_H

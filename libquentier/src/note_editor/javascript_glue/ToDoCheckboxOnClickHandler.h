#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TO_DO_CHECKBOX_ON_CLICK_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TO_DO_CHECKBOX_ON_CLICK_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TO_DO_CHECKBOX_ON_CLICK_HANDLER_H

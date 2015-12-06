#include "ToDoCheckboxOnClickHandler.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ToDoCheckboxOnClickHandler::ToDoCheckboxOnClickHandler(QObject * parent) :
    QObject(parent)
{}

void ToDoCheckboxOnClickHandler::onToDoCheckboxClicked(QString enToDoCheckboxId)
{
    QNDEBUG("ToDoCheckboxOnClickHandler::onToDoCheckboxClicked: " << enToDoCheckboxId);

    bool conversionResult = false;
    quint64 id = enToDoCheckboxId.toULongLong(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QString error = QT_TR_NOOP("Error handling todo checkbox click event: can't convert id from string to number");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit toDoCheckboxClicked(id);
}

} // namespace qute_note

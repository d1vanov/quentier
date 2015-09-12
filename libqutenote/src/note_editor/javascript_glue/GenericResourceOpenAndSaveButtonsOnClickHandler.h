#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

class GenericResourceOpenAndSaveButtonsOnClickHandler: public QObject
{
    Q_OBJECT
public:
    explicit GenericResourceOpenAndSaveButtonsOnClickHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void openResourceRequest(const QString & resourceHash);
    void saveResourceRequest(const QString & resourceHash);

public Q_SLOTS:
    void onOpenResourceButtonPressed(const QString & resourceHash);
    void onSaveResourceButtonPressed(const QString & resourceHash);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H

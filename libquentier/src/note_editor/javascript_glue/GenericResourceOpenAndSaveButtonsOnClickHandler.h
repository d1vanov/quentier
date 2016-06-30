#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>

namespace quentier {

class GenericResourceOpenAndSaveButtonsOnClickHandler: public QObject
{
    Q_OBJECT
public:
    explicit GenericResourceOpenAndSaveButtonsOnClickHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void openResourceRequest(const QByteArray & resourceHash);
    void saveResourceRequest(const QByteArray & resourceHash);

public Q_SLOTS:
    void onOpenResourceButtonPressed(const QString & resourceHash);
    void onSaveResourceButtonPressed(const QString & resourceHash);
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H

#ifndef LIB_QUENTIER_NOTE_EDITOR_HYPERLINK_CLICK_JAVA_SCRIPT_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_HYPERLINK_CLICK_JAVA_SCRIPT_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>
#include <QString>

namespace quentier {

class HyperlinkClickJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit HyperlinkClickJavaScriptHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void hyperlinkClicked(QString url);

public Q_SLOTS:
    void onHyperlinkClicked(QString url);
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_HYPERLINK_CLICK_JAVA_SCRIPT_HANDLER_H

#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_HYPERLINK_CLICK_JAVA_SCRIPT_HANDLER_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_HYPERLINK_CLICK_JAVA_SCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QString>

namespace qute_note {

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

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_HYPERLINK_CLICK_JAVA_SCRIPT_HANDLER_H

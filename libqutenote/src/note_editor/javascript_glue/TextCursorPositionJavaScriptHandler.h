#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

class TextCursorPositionJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit TextCursorPositionJavaScriptHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void textCursorPositionChanged();

public Q_SLOTS:
    void onTextCursorPositionChange();
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H

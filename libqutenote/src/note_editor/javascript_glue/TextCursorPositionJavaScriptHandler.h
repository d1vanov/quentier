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

    void textCursorPositionBoldState(bool bold);
    void textCursorPositionItalicState(bool italic);
    void textCursorPositionUnderlineState(bool underline);
    void textCursorPositionStrikethgouthState(bool strikethrough);

    void textCursorPositionLeftAlignment();
    void textCursorPositionCenterAlignment();
    void textCursorPositionRightAlignment();

    void textCursorPositionInsideOrderedListState(bool insideOrderedList);
    void textCursorPositionInsideUnorderedListState(bool insideUnorderedList);

    void textCursorPositionInsideTableState(bool insideTable);

public Q_SLOTS:
    void onTextCursorPositionChange();

    void setTextCursorPositionBoldState(bool bold);
    void setTextCursorPositionItalicState(bool italic);
    void setTextCursorPositionUnderlineState(bool underline);
    void setTextCursorPositionStrikethroughState(bool strikethrough);

    void setTextCursorPositionLeftAlignment();
    void setTextCursorPositionCenterAlignment();
    void setTextCursorPositionRightAlignment();

    void setTextCursorPositionInsideOrderedListState(bool insideOrderedList);
    void setTextCursorPositionInsideUnorderedListState(bool insideUnorderedList);

    void setTextCursorPositionInsideTableState(bool insideTable);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H

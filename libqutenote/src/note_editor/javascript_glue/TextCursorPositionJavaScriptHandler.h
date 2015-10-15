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

    void textCursorPositionOnImageResourceState(bool state, QString resourceHash);
    void textCursorPositionOnNonImageResourceState(bool state, QString resourceHash);
    void textCursorPositionOnEnCryptTagState(bool state, QString encryptedText, QString cipher, QString length);

    void textCursorPositionBoldState(bool bold);
    void textCursorPositionItalicState(bool italic);
    void textCursorPositionUnderlineState(bool underline);
    void textCursorPositionStrikethgouthState(bool strikethrough);

    void textCursorPositionAlignLeftState(bool state);
    void textCursorPositionAlignCenterState(bool state);
    void textCursorPositionAlignRightState(bool state);

    void textCursorPositionInsideOrderedListState(bool insideOrderedList);
    void textCursorPositionInsideUnorderedListState(bool insideUnorderedList);

    void textCursorPositionInsideTableState(bool insideTable);

    void textCursorPositionFontName(QString fontName);
    void textCursorPositionFontSize(int fontSize);

public Q_SLOTS:
    void onTextCursorPositionChange();

    void setOnImageResourceState(bool state, QString resourceHash);
    void setOnNonImageResourceState(bool state, QString resourceHash);
    void setOnEnCryptTagState(bool state, QString encryptedText, QString cipher, QString length);

    void setTextCursorPositionBoldState(bool bold);
    void setTextCursorPositionItalicState(bool italic);
    void setTextCursorPositionUnderlineState(bool underline);
    void setTextCursorPositionStrikethroughState(bool strikethrough);

    void setTextCursorPositionAlignLeftState(bool state);
    void setTextCursorPositionAlignCenterState(bool state);
    void setTextCursorPositionAlignRightState(bool state);

    void setTextCursorPositionInsideOrderedListState(bool insideOrderedList);
    void setTextCursorPositionInsideUnorderedListState(bool insideUnorderedList);

    void setTextCursorPositionInsideTableState(bool insideTable);

    void setTextCursorPositionFontName(QString name);
    void setTextCursorPositionFontSize(int fontSize);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H

#include "TextCursorPositionJavaScriptHandler.h"

namespace qute_note {

TextCursorPositionJavaScriptHandler::TextCursorPositionJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void TextCursorPositionJavaScriptHandler::onTextCursorPositionChange()
{
    emit textCursorPositionChanged();
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionBoldState(bool bold)
{
    emit textCursorPositionBoldState(bold);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionItalicState(bool italic)
{
    emit textCursorPositionItalicState(italic);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionUnderlineState(bool underline)
{
    emit textCursorPositionUnderlineState(underline);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionStrikethroughState(bool strikethrough)
{
    emit textCursorPositionStrikethgouthState(strikethrough);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionAlignLeftState(bool state)
{
    emit textCursorPositionAlignLeftState(state);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionAlignCenterState(bool state)
{
    emit textCursorPositionAlignCenterState(state);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionAlignRightState(bool state)
{
    emit textCursorPositionAlignRightState(state);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionInsideOrderedListState(bool insideOrderedList)
{
    emit textCursorPositionInsideOrderedListState(insideOrderedList);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionInsideUnorderedListState(bool insideUnorderedList)
{
    emit textCursorPositionInsideUnorderedListState(insideUnorderedList);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionInsideTableState(bool insideTable)
{
    emit textCursorPositionInsideTableState(insideTable);
}

} // namespace qute_note

#include "TextCursorPositionJavaScriptHandler.h"

namespace qute_note {

TextCursorPositionJavaScriptHandler::TextCursorPositionJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void TextCursorPositionJavaScriptHandler::onTextCursorPositionChange()
{
    emit textCursorPositionChanged();
}

void TextCursorPositionJavaScriptHandler::setOnImageResourceState(bool state, QString resourceHash)
{
    emit textCursorPositionOnImageResourceState(state, resourceHash);
}

void TextCursorPositionJavaScriptHandler::setOnNonImageResourceState(bool state, QString resourceHash)
{
    emit textCursorPositionOnNonImageResourceState(state, resourceHash);
}

void TextCursorPositionJavaScriptHandler::setOnEnCryptTagState(bool state, QString encryptedText, QString cipher, QString length)
{
    emit textCursorPositionOnEnCryptTagState(state, encryptedText, cipher, length);
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
    emit textCursorPositionStrikethroughState(strikethrough);
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

void TextCursorPositionJavaScriptHandler::setTextCursorPositionFontName(QString fontSize)
{
    emit textCursorPositionFontName(fontSize);
}

void TextCursorPositionJavaScriptHandler::setTextCursorPositionFontSize(int fontSize)
{
    emit textCursorPositionFontSize(fontSize);
}

} // namespace qute_note

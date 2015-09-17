#include "TextCursorPositionJavaScriptHandler.h"

namespace qute_note {

TextCursorPositionJavaScriptHandler::TextCursorPositionJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void TextCursorPositionJavaScriptHandler::onTextCursorPositionChange()
{
    emit textCursorPositionChanged();
}

} // namespace qute_note

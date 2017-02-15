/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TextCursorPositionJavaScriptHandler.h"

namespace quentier {

TextCursorPositionJavaScriptHandler::TextCursorPositionJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void TextCursorPositionJavaScriptHandler::onTextCursorPositionChange()
{
    emit textCursorPositionChanged();
}

void TextCursorPositionJavaScriptHandler::setOnImageResourceState(bool state, QString resourceHash)
{
    emit textCursorPositionOnImageResourceState(state, QByteArray::fromHex(resourceHash.toLocal8Bit()));
}

void TextCursorPositionJavaScriptHandler::setOnNonImageResourceState(bool state, QString resourceHash)
{
    emit textCursorPositionOnNonImageResourceState(state, QByteArray::fromHex(resourceHash.toLocal8Bit()));
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

void TextCursorPositionJavaScriptHandler::setTextCursorPositionAlignFullState(bool state)
{
    emit textCursorPositionAlignFullState(state);
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

} // namespace quentier

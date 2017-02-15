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

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H

#include <quentier/utility/Macros.h>
#include <QObject>

namespace quentier {

class TextCursorPositionJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit TextCursorPositionJavaScriptHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void textCursorPositionChanged();

    void textCursorPositionOnImageResourceState(bool state, QByteArray resourceHash);
    void textCursorPositionOnNonImageResourceState(bool state, QByteArray resourceHash);
    void textCursorPositionOnEnCryptTagState(bool state, QString encryptedText, QString cipher, QString length);

    void textCursorPositionBoldState(bool bold);
    void textCursorPositionItalicState(bool italic);
    void textCursorPositionUnderlineState(bool underline);
    void textCursorPositionStrikethroughState(bool strikethrough);

    void textCursorPositionAlignLeftState(bool state);
    void textCursorPositionAlignCenterState(bool state);
    void textCursorPositionAlignRightState(bool state);
    void textCursorPositionAlignFullState(bool state);

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
    void setTextCursorPositionAlignFullState(bool state);

    void setTextCursorPositionInsideOrderedListState(bool insideOrderedList);
    void setTextCursorPositionInsideUnorderedListState(bool insideUnorderedList);

    void setTextCursorPositionInsideTableState(bool insideTable);

    void setTextCursorPositionFontName(QString name);
    void setTextCursorPositionFontSize(int fontSize);
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TEXT_CURSOR_POSITION_JAVASCRIPT_HANDLER_H

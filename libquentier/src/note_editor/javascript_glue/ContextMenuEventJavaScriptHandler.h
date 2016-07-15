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

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_CONTEXT_MENU_EVENT_JAVA_SCRIPT_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_CONTEXT_MENU_EVENT_JAVA_SCRIPT_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>
#include <QStringList>

namespace quentier {

/**
 * @brief The ContextMenuEventJavaScriptHandler class represents an object interacting with JavaScript code
 * when it is asked to assist with the processing of context menu event.
 *
 * JavaScript needs to identify the type of object the context menu referred which is assumed to be the type of object
 * being currently under cursor. If it's text/html, the currently selected piece of it is passed
 * to C++ code as well. The JavaScript calls public slot setContextMenuContent of this object which in turn
 * relays this information further via signal contextMenuEventReply to anyone connected to that signal.
 *
 * The sequence number is used by these signals to ensure the possibility to find out which context menu reply
 * was given for which events. For example, for ignoring the replies for all but the last context menu event.
 */
class ContextMenuEventJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit ContextMenuEventJavaScriptHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void contextMenuEventReply(QString contentType, QString selectedHtml, bool insideDecryptedTextFragment,
                               QStringList extraData, quint64 sequenceNumber);

public Q_SLOTS:
    void setContextMenuContent(QString contentType, QString selectedHtml, bool insideDecryptedTextFragment,
                               QStringList extraData, quint64 sequenceNumber);
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_CONTEXT_MENU_EVENT_JAVA_SCRIPT_HANDLER_H

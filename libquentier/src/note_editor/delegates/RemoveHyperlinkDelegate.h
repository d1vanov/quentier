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

#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATE_REMOVE_HYPERLINK_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATE_REMOVE_HYPERLINK_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/types/Note.h>
#include <quentier/types/ErrorString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

/**
 * @brief The RemoveHyperlinkDelegate class encapsulates a chain of callbacks
 * required for proper implementation of removing a hyperlink under cursor
 * considering the details of wrapping this action around the undo stack
 */
class RemoveHyperlinkDelegate: public QObject
{
    Q_OBJECT
public:
    explicit RemoveHyperlinkDelegate(NoteEditorPrivate & noteEditor);

    void start();

Q_SIGNALS:
    void finished();
    void notifyError(ErrorString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onHyperlinkIdFound(const QVariant & data);
    void onHyperlinkRemoved(const QVariant & data);

private:
    void findIdOfHyperlinkUnderCursor();
    void removeHyperlink(const quint64 hyperlinkId);

private:
    typedef JsResultCallbackFunctor<RemoveHyperlinkDelegate> JsCallback;

private:
    NoteEditorPrivate &     m_noteEditor;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATE_REMOVE_HYPERLINK_DELEGATE_H

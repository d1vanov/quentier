/*
 * Copyright 2017-2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * The contents of this file were heavily inspired by the source code of Qt
 * Creator, a Qt and C/C++ IDE by The Qt Company Ltd., 2016. That source code is
 * licensed under GNU GPL v.3 license with two permissive exceptions although
 * they don't apply to this derived work.
 */

#include "ShortcutButton.h"

#include <quentier/logging/QuentierLogger.h>

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>

namespace quentier {

namespace {

[[nodiscard]] int translateModifiers(
    Qt::KeyboardModifiers state, const QString & text)
{
    int result = 0;
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key anyway
    if ((state & Qt::ShiftModifier) &&
        ((text.size() == 0) || !text.at(0).isPrint() ||
         text.at(0).isLetterOrNumber() || text.at(0).isSpace()))
    {
        result |= Qt::SHIFT;
    }

    if (state & Qt::ControlModifier) {
        result |= Qt::CTRL;
    }

    if (state & Qt::MetaModifier) {
        result |= Qt::META;
    }

    if (state & Qt::AltModifier) {
        result |= Qt::ALT;
    }

    return result;
}

} // namespace

ShortcutButton::ShortcutButton(QWidget * parent) : QPushButton(parent)
{
    setToolTip(tr("Click and type the new key sequence."));
    setCheckable(true);

    m_checkedText = tr("Stop recording");
    m_uncheckedText = tr("Record");
    updateText();

    QObject::connect(
        this, &ShortcutButton::toggled, this,
        &ShortcutButton::handleToggleChange);
}

QSize ShortcutButton::sizeHint() const
{
    if (m_preferredWidth < 0) {
        // Initialize size hint

        const QString originalText = text();

        auto * that = const_cast<ShortcutButton *>(this);
        that->setText(m_checkedText);

        m_preferredWidth = QPushButton::sizeHint().width();
        that->setText(m_uncheckedText);

        const int otherWidth = QPushButton::sizeHint().width();
        if (otherWidth > m_preferredWidth) {
            m_preferredWidth = otherWidth;
        }

        that->setText(originalText);
    }

    return QSize{m_preferredWidth, QPushButton::sizeHint().height()};
}

bool ShortcutButton::eventFilter(QObject * watched, QEvent * event)
{
    if (Q_UNLIKELY(!event)) {
        return true;
    }

    const auto eventType = event->type();
    if (eventType == QEvent::ShortcutOverride) {
        QNDEBUG(
            "preferences",
            "ShortcutButton: detected shortcut override event");
        event->accept();
        return true;
    }

    if (eventType == QEvent::KeyRelease || eventType == QEvent::Shortcut ||
        eventType == QEvent::Close) // Escape tries to close the dialog
    {
        return true;
    }

    if (eventType == QEvent::MouseButtonPress && isChecked()) {
        setChecked(false);
        return true;
    }

    if (eventType == QEvent::KeyPress) {
        auto * keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (Q_UNLIKELY(!keyEvent)) {
            QNWARNING(
                "preferences",
                "Failed to cast QEvent of KeyPress type to QKeyEvent");
            return true;
        }

        int nextKey = keyEvent->key();
        if (m_keyNum > 3 || nextKey == Qt::Key_Control ||
            nextKey == Qt::Key_Shift || nextKey == Qt::Key_Meta ||
            nextKey == Qt::Key_Alt)
        {
            return false;
        }

        nextKey |=
            translateModifiers(keyEvent->modifiers(), keyEvent->text());

        switch (m_keyNum) {
        case 0:
            m_key[0] = nextKey;
            break;
        case 1:
            m_key[1] = nextKey;
            break;
        case 2:
            m_key[2] = nextKey;
            break;
        case 3:
            m_key[3] = nextKey;
            break;
        default:
            break;
        }

        ++m_keyNum;
        keyEvent->accept();

        Q_EMIT keySequenceChanged(
            QKeySequence{m_key[0], m_key[1], m_key[2], m_key[3]});

        if (m_keyNum > 3) {
            setChecked(false);
        }

        return true;
    }

    return QPushButton::eventFilter(watched, event);
}

void ShortcutButton::updateText()
{
    setText(isChecked() ? m_checkedText : m_uncheckedText);
}

void ShortcutButton::handleToggleChange(bool toggleChange)
{
    updateText();

    for (std::size_t i = 0; i < m_key.size(); ++i) {
        m_key[i] = 0;
    }
    m_keyNum = 0;

    if (toggleChange) {
        QWidget * focusWidget = QApplication::focusWidget();
        if (focusWidget) {
            focusWidget->clearFocus(); // funny things happen otherwise
        }

        qApp->installEventFilter(this);
    }
    else {
        qApp->removeEventFilter(this);
    }
}

} // namespace quentier

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

#pragma once

#include <lib/model/log_viewer/LogViewerModel.h>

#include <QStyledItemDelegate>

namespace quentier {

class LogViewerDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
public:
    LogViewerDelegate(QObject * parent = nullptr);

    [[nodiscard]] static constexpr int maxSourceFileNameColumnWidth()
    {
        return 200;
    }

private:
    // QStyledItemDelegate interface
    [[nodiscard]] QWidget * createEditor(
        QWidget * parent, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    void paint(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    [[nodiscard]] QSize sizeHint(
        const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

private:
    [[nodiscard]] bool paintImpl(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const;

    void paintLogEntry(
        QPainter & painter, const QRect & adjustedRect,
        const LogViewerModel::Data & dataEntry,
        const QFontMetrics & fontMetrics) const;

private:
    const QChar m_newlineChar;
    const QChar m_whitespaceChar;
    const double m_margin;
    const QString m_widestLogLevelName;
    const QString m_sampleDateTimeString;
    const QString m_sampleSourceFileLineNumberString;
};

} // namespace quentier

/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DELEGATE_LOG_VIEWER_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_LOG_VIEWER_DELEGATE_H

#include <lib/model/log_viewer/LogViewerModel.h>

#include <QStyledItemDelegate>

#define MAX_SOURCE_FILE_NAME_COLUMN_WIDTH (200)

namespace quentier {

class LogViewerDelegate final: public QStyledItemDelegate
{
    Q_OBJECT
public:
    LogViewerDelegate(QObject * parent = nullptr);

private:
    // QStyledItemDelegate interface
    virtual QWidget * createEditor(
        QWidget * pParent, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    virtual void paint(
        QPainter * pPainter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    virtual QSize sizeHint(
        const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

private:
    bool paintImpl(
        QPainter * pPainter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const;

    void paintLogEntry(
        QPainter & painter, const QRect & adjustedRect,
        const LogViewerModel::Data & dataEntry,
        const QFontMetrics & fontMetrics) const;

private:
    double m_margin;
    QString m_widestLogLevelName;
    QString m_sampleDateTimeString;
    QString m_sampleSourceFileLineNumberString;

    QChar m_newlineChar;
    QChar m_whitespaceChar;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_LOG_VIEWER_DELEGATE_H

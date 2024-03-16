/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include <QString>
#include <QWidget>

namespace Ui {

class FindAndReplaceWidget;

} // namespace Ui

namespace quentier {

class FindAndReplaceWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit FindAndReplaceWidget(
        QWidget * parent = nullptr, bool withReplace = false);

    ~FindAndReplaceWidget() override;

    [[nodiscard]] QString textToFind() const;
    void setTextToFind(const QString & text);

    [[nodiscard]] QString replacementText() const;
    void setReplacementText(const QString & text);

    [[nodiscard]] bool matchCase() const;
    void setMatchCase(const bool flag);

    [[nodiscard]] bool replaceEnabled() const;
    void setReplaceEnabled(const bool enabled);

public:
    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

Q_SIGNALS:
    void closed();
    void textToFindEdited(const QString & textToFind);
    void findNext(const QString & textToFind, const bool matchCase);
    void findPrevious(const QString & textToFind, const bool matchCase);
    void searchCaseSensitivityChanged(const bool matchCase);

    void replace(
        const QString & textToReplace, const QString & replacementText,
        bool matchCase);

    void replaceAll(
        const QString & textToReplace, const QString & replacementText,
        bool matchCase);

public Q_SLOTS:
    void setFocus();
    void show();

private Q_SLOTS:
    void onCloseButtonPressed();
    void onFindTextEntered();
    void onNextButtonPressed();
    void onPreviousButtonPressed();
    void onMatchCaseCheckboxToggled(int state);
    void onReplaceTextEntered();
    void onReplaceButtonPressed();
    void onReplaceAllButtonPressed();

private:
    void createConnections();
    [[nodiscard]] QSize sizeHintImpl(const bool minimal) const;

private:
    Ui::FindAndReplaceWidget * m_ui;
};

} // namespace quentier

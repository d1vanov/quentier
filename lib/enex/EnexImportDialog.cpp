/*
 * Copyright 2017-2025 Dmitry Ivanov
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

#include "EnexImportDialog.h"
#include "ui_EnexImportDialog.h"

#include <lib/model/notebook/NotebookModel.h>
#include <lib/preferences/keys/Enex.h>
#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>

#include <QCompleter>
#include <QFileDialog>
#include <QFileInfo>
#include <QModelIndex>
#include <QStringListModel>

#include <algorithm>
#include <memory>

namespace quentier {

EnexImportDialog::EnexImportDialog(
    Account account, NotebookModel & notebookModel, QWidget * parent) :
    QDialog{parent}, m_currentAccount{std::move(account)},
    m_ui{new Ui::EnexImportDialog},
    m_notebookModel{&notebookModel},
    m_notebookNamesModel{new QStringListModel(this)}
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Import ENEX"));

    fillNotebookNames();
    m_ui->notebookNameComboBox->setModel(m_notebookNamesModel);

    if (auto * completer = m_ui->notebookNameComboBox->completer()) {
        completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    }

    fillDialogContents();
    createConnections();
}

EnexImportDialog::~EnexImportDialog()
{
    delete m_ui;
}

QString EnexImportDialog::importEnexFilePath(
    ErrorString * errorDescription) const
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::importEnexFilePath");

    const QString currentFilePath =
        QDir::fromNativeSeparators(m_ui->filePathLineEdit->text());

    QNTRACE("enex::EnexImportDialog", "Current file path: " << currentFilePath);

    if (currentFilePath.isEmpty()) {
        return {};
    }

    const QFileInfo fileInfo{currentFilePath};
    if (!fileInfo.exists()) {
        QNDEBUG(
            "enex::EnexImportDialog",
            "ENEX file at specified path doesn't exist");
        if (errorDescription) {
            errorDescription->setBase(
                QT_TR_NOOP("ENEX file at specified path doesn't exist"));
        }

        return {};
    }

    if (!fileInfo.isFile()) {
        QNDEBUG("enex::EnexImportDialog", "The specified path is not a file");
        if (errorDescription) {
            errorDescription->setBase(
                QT_TR_NOOP("The specified path is not a file"));
        }

        return {};
    }

    if (!fileInfo.isReadable()) {
        QNDEBUG("enex::EnexImportDialog", "The specified file is not readable");
        if (errorDescription) {
            errorDescription->setBase(
                QT_TR_NOOP("The specified file is not readable"));
        }

        return {};
    }

    return currentFilePath;
}

QString EnexImportDialog::notebookName(ErrorString * errorDescription) const
{
    QString currentNotebookName = m_ui->notebookNameComboBox->currentText();
    if (currentNotebookName.isEmpty()) {
        return {};
    }

    if (!validateNotebookName(currentNotebookName, errorDescription)) {
        return {};
    }

    return currentNotebookName;
}

void EnexImportDialog::onBrowsePushButtonClicked()
{
    QNDEBUG(
        "enex::EnexImportDialog",
        "EnexImportDialog::onBrowsePushButtonClicked");

    utility::ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::auxiliary};

    QString lastEnexImportPath;
    {
        appSettings.beginGroup(preferences::keys::enexExportImportGroup);
        utility::ApplicationSettings::GroupCloser groupCloser{appSettings};

        lastEnexImportPath =
            appSettings.value(preferences::keys::lastEnexImportPath).toString();
    }

    if (lastEnexImportPath.isEmpty()) {
        lastEnexImportPath = documentsPath();
    }

    auto enexFileDialog = std::make_unique<QFileDialog>(
        this, tr("Please select the ENEX file to import"), lastEnexImportPath);

    enexFileDialog->setWindowModality(Qt::WindowModal);
    enexFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    enexFileDialog->setFileMode(QFileDialog::ExistingFile);
    enexFileDialog->setDefaultSuffix(QStringLiteral("enex::EnexImportDialog"));

    if (enexFileDialog->exec() != QDialog::Accepted) {
        QNDEBUG("enex::EnexImportDialog", "The import of ENEX was cancelled");
        return;
    }

    const auto selectedFiles = enexFileDialog->selectedFiles();
    const auto numSelectedFiles = selectedFiles.size();

    if (numSelectedFiles == 0) {
        QNDEBUG("enex::EnexImportDialog", "No ENEX file was selected");
        setStatusText(tr("No ENEX file was selected"));
        return;
    }

    if (numSelectedFiles > 1) {
        QNDEBUG(
            "enex::EnexImportDialog", "More than one ENEX files were selected");
        setStatusText(tr("More than one ENEX files were selected"));
        return;
    }

    const QFileInfo enexFileInfo{selectedFiles[0]};
    if (!enexFileInfo.exists()) {
        QNDEBUG(
            "enex::EnexImportDialog", "The selected ENEX file does not exist");
        setStatusText(tr("The selected ENEX file does not exist"));
        return;
    }

    if (!enexFileInfo.isReadable()) {
        QNDEBUG("enex::EnexImportDialog", "The selected ENEX file is not readable");
        setStatusText(tr("The selected ENEX file is not readable"));
        return;
    }

    lastEnexImportPath = enexFileDialog->directory().absolutePath();

    if (!lastEnexImportPath.isEmpty()) {
        appSettings.beginGroup(preferences::keys::enexExportImportGroup);
        utility::ApplicationSettings::GroupCloser groupCloser{appSettings};

        appSettings.setValue(
            preferences::keys::lastEnexImportPath, lastEnexImportPath);
    }

    m_ui->filePathLineEdit->setText(
        QDir::toNativeSeparators(enexFileInfo.absoluteFilePath()));

    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::onNotebookIndexChanged(int notebookNameIndex)
{
    onNotebookNameEdited(
        m_ui->notebookNameComboBox->itemText(notebookNameIndex));
}

void EnexImportDialog::onNotebookNameEdited(const QString & name)
{
    QNDEBUG(
        "enex::EnexImportDialog",
        "EnexImportDialog::onNotebookNameEdited: " << name);

    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::onEnexFilePathEdited(const QString & path)
{
    QNDEBUG(
        "enex::EnexImportDialog",
        "EnexImportDialog::onEnexFilePathEdited: " << path);

    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::dataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    [[maybe_unused]] const QVector<int> & roles)
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::dataChanged");

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        QNDEBUG(
            "enex::EnexImportDialog",
            "At least one of changed indexes is invalid");
        fillNotebookNames();
        return;
    }

    if (topLeft.column() > static_cast<int>(NotebookModel::Column::Name) ||
        bottomRight.column() < static_cast<int>(NotebookModel::Column::Name))
    {
        QNTRACE(
            "enex::EnexImportDialog",
            "The updated indexed don't contain the notebook name");
        return;
    }

    fillNotebookNames();
}

void EnexImportDialog::rowsInserted(
    [[maybe_unused]] const QModelIndex & parent,
    [[maybe_unused]] const int start, [[maybe_unused]] const int end)
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::rowsInserted");
    fillNotebookNames();
}

void EnexImportDialog::rowsAboutToBeRemoved(
    const QModelIndex & parent, const int start, const int end)
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::rowsAboutToBeRemoved");

    if (Q_UNLIKELY(m_notebookModel.isNull())) {
        QNDEBUG(
            "enex::EnexImportDialog", "Notebook model is null, nothing to do");
        return;
    }

    auto currentNotebookNames = m_notebookNamesModel->stringList();

    for (int i = start; i <= end; ++i) {
        const auto index = m_notebookModel->index(
            i, static_cast<int>(NotebookModel::Column::Name), parent);

        const QString removedNotebookName =
            m_notebookModel->data(index).toString();
        if (Q_UNLIKELY(removedNotebookName.isEmpty())) {
            continue;
        }

        const auto it = std::lower_bound(
            currentNotebookNames.constBegin(), currentNotebookNames.constEnd(),
            removedNotebookName);

        if (it != currentNotebookNames.constEnd() && *it == removedNotebookName)
        {
            const int offset = static_cast<int>(
                std::distance(currentNotebookNames.constBegin(), it));

            const auto nit = currentNotebookNames.begin() + offset;
            currentNotebookNames.erase(nit);
        }
    }

    m_notebookNamesModel->setStringList(currentNotebookNames);
}

void EnexImportDialog::accept()
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::accept");

    const QString notebookName = m_ui->notebookNameComboBox->currentText();
    {
        utility::ApplicationSettings appSettings{
            m_currentAccount, preferences::keys::files::auxiliary};

        appSettings.beginGroup(preferences::keys::enexExportImportGroup);
        utility::ApplicationSettings::GroupCloser groupCloser{appSettings};

        appSettings.setValue(
            preferences::keys::lastImportEnexNotebookName, notebookName);
    }

    QDialog::accept();
}

void EnexImportDialog::createConnections()
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::createConnections");

    QObject::connect(
        m_ui->browsePushButton, &QPushButton::clicked, this,
        &EnexImportDialog::onBrowsePushButtonClicked);

    QObject::connect(
        m_ui->filePathLineEdit, &QLineEdit::textEdited, this,
        &EnexImportDialog::onEnexFilePathEdited);

    QObject::connect(
        m_ui->notebookNameComboBox, &QComboBox::editTextChanged, this,
        &EnexImportDialog::onNotebookNameEdited);

    QObject::connect(
        m_ui->notebookNameComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &EnexImportDialog::onNotebookIndexChanged);

    if (!m_notebookModel.isNull()) {
        QObject::connect(
            m_notebookModel.data(), &NotebookModel::dataChanged, this,
            &EnexImportDialog::dataChanged);

        QObject::connect(
            m_notebookModel.data(), &NotebookModel::rowsInserted, this,
            &EnexImportDialog::rowsInserted);

        QObject::connect(
            m_notebookModel.data(), &NotebookModel::rowsAboutToBeRemoved, this,
            &EnexImportDialog::rowsAboutToBeRemoved);
    }
}

void EnexImportDialog::fillNotebookNames()
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::fillNotebookNames");

    QStringList notebookNames;

    if (!m_notebookModel.isNull()) {
        NotebookModel::Filters filter(NotebookModel::Filter::CanCreateNotes);
        notebookNames = m_notebookModel->notebookNames(filter);
    }

    m_notebookNamesModel->setStringList(notebookNames);
}

void EnexImportDialog::fillDialogContents()
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::fillDialogContents");

    QString lastImportEnexNotebookName;
    {
        utility::ApplicationSettings appSettings{
            m_currentAccount, preferences::keys::files::auxiliary};

        appSettings.beginGroup(preferences::keys::enexExportImportGroup);
        utility::ApplicationSettings::GroupCloser groupCloser{appSettings};

        lastImportEnexNotebookName =
            appSettings.value(preferences::keys::lastImportEnexNotebookName)
                .toString();
    }

    if (lastImportEnexNotebookName.isEmpty()) {
        lastImportEnexNotebookName = tr("Imported notes");
    }

    const auto notebookNames = m_notebookNamesModel->stringList();
    const auto it = std::lower_bound(
        notebookNames.constBegin(), notebookNames.constEnd(),
        lastImportEnexNotebookName);

    if ((it != notebookNames.constEnd()) && (*it == lastImportEnexNotebookName))
    {
        const int notebookNameIndex =
            static_cast<int>(std::distance(notebookNames.constBegin(), it));

        m_ui->notebookNameComboBox->setCurrentIndex(notebookNameIndex);
    }
    else {
        m_ui->notebookNameComboBox->setEditText(lastImportEnexNotebookName);
    }

    m_ui->statusTextLabel->setHidden(true);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
}

void EnexImportDialog::setStatusText(const QString & text)
{
    m_ui->statusTextLabel->setText(text);
    m_ui->statusTextLabel->setHidden(false);
}

void EnexImportDialog::clearAndHideStatus()
{
    QNDEBUG("enex::EnexImportDialog", "EnexImportDialog::clearAndHideStatus");

    m_ui->statusTextLabel->setText(QString());
    m_ui->statusTextLabel->setHidden(true);
}

void EnexImportDialog::checkConditionsAndEnableDisableOkButton()
{
    QNDEBUG(
        "enex::EnexImportDialog",
        "EnexImportDialog::checkConditionsAndEnableDisableOkButton");

    ErrorString error;
    const QString enexFilePath = importEnexFilePath(&error);
    if (enexFilePath.isEmpty()) {
        QNDEBUG(
            "enex::EnexImportDialog",
            "The enex file path is invalid, disabling the ok button");
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        setStatusText(error.localizedString());
        return;
    }

    const QString currentNotebookName = notebookName(&error);
    if (currentNotebookName.isEmpty()) {
        QNDEBUG(
            "enex::EnexImportDialog",
            "Notebook name is not set or is not valid, disabling the ok "
                << "button");
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        setStatusText(error.localizedString());
        return;
    }

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
    clearAndHideStatus();
}

} // namespace quentier

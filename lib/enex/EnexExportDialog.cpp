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

#include "EnexExportDialog.h"
#include "ui_EnexExportDialog.h"

#include <lib/preferences/keys/Enex.h>
#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

#include <memory>

namespace quentier {

EnexExportDialog::EnexExportDialog(
    Account account, QWidget * parent,
    const QString & suggestedFileName) :
    QDialog{parent}, m_currentAccount{std::move(account)},
    m_ui{new Ui::EnexExportDialog}
{
    m_ui->setupUi(this);

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::auxiliary};

    QString lastEnexExportPath;
    bool exportTags = false;

    {
        appSettings.beginGroup(preferences::keys::enexExportImportGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};

        lastEnexExportPath =
            appSettings.value(preferences::keys::lastExportNotesToEnexPath)
                .toString();

        exportTags =
            (appSettings.contains(
                 preferences::keys::lastExportNotesToEnexExportTags)
                 ? appSettings
                       .value(
                           preferences::keys::lastExportNotesToEnexExportTags)
                       .toBool()
                 : true);
    }

    if (lastEnexExportPath.isEmpty()) {
        lastEnexExportPath = documentsPath();
    }

    QFileInfo lastEnexExportPathInfo{lastEnexExportPath};
    if (lastEnexExportPathInfo.exists() && lastEnexExportPathInfo.isDir() &&
        lastEnexExportPathInfo.isWritable())
    {
        m_ui->folderLineEdit->setText(lastEnexExportPath);
        QNDEBUG(
            "enex",
            "EnexExportDialog: initialized folder path to "
                << lastEnexExportPath);
    }

    m_ui->exportTagsCheckBox->setChecked(exportTags);
    m_ui->fileNameLineEdit->setText(suggestedFileName);
    m_ui->statusTextLabel->setHidden(true);

    createConnections();
    checkConditionsAndEnableDisableOkButton();
}

EnexExportDialog::~EnexExportDialog()
{
    delete m_ui;
}

bool EnexExportDialog::exportTags() const noexcept
{
    return m_ui->exportTagsCheckBox->isChecked();
}

QString EnexExportDialog::exportEnexFilePath() const
{
    const QString folderPath = m_ui->folderLineEdit->text();
    QString convertedFolderPath = QDir::fromNativeSeparators(folderPath);
    QString fileName = m_ui->fileNameLineEdit->text();

    QNDEBUG(
        "enex",
        "EnexExportDialog::exportEnexFilePath: folder path = "
            << folderPath << ", converted folder path = " << convertedFolderPath
            << ", file name = " << fileName);

    if (convertedFolderPath.isEmpty()) {
        QNDEBUG("enex", "No folder path was specified");
        return {};
    }

    if (fileName.isEmpty()) {
        QNDEBUG("enex", "No file name was specified");
        return {};
    }

    QFileInfo dirInfo{convertedFolderPath};
    if (!dirInfo.exists()) {
        QNDEBUG("enex", "Directory doesn't exist");
        return {};
    }

    if (!dirInfo.isDir()) {
        QNDEBUG("enex", "Non-directory was selected");
        return {};
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG("enex", "Non-writable directory was selected");
        return {};
    }

    if (!convertedFolderPath.endsWith(QStringLiteral("/"))) {
        convertedFolderPath += QStringLiteral("/");
    }

    if (!fileName.endsWith(QStringLiteral(".enex"), Qt::CaseInsensitive)) {
        fileName += QStringLiteral(".enex");
    }

    convertedFolderPath += fileName;
    QNDEBUG("enex", "Absolute ENEX export file path: " << convertedFolderPath);
    return convertedFolderPath;
}

void EnexExportDialog::onExportTagsOptionChanged(int state)
{
    QNDEBUG(
        "enex",
        "EnexExportDialog::onExportTagsOptionChanged: state = " << state);

    const bool checked = (state == Qt::Checked);

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::auxiliary};

    {
        appSettings.beginGroup(preferences::keys::enexExportImportGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};

        appSettings.setValue(
            preferences::keys::lastExportNotesToEnexExportTags, checked);
    }

    Q_EMIT exportTagsOptionChanged(checked);
}

void EnexExportDialog::onBrowseFolderButtonPressed()
{
    QNDEBUG("enex", "EnexExportDialog::onBrowseFolderButtonPressed");

    clearAndHideStatus();

    QString currentExportDir = m_ui->folderLineEdit->text();
    if (!currentExportDir.isEmpty()) {
        const QFileInfo currentExportDirInfo{currentExportDir};
        if (!currentExportDirInfo.exists() || !currentExportDirInfo.isDir()) {
            currentExportDir.clear();
        }
    }

    auto pFileDialog = std::make_unique<QFileDialog>(
        this,
        tr("Please select the folder in which to store the output ENEX file"),
        currentExportDir);

    pFileDialog->setWindowModality(Qt::WindowModal);
    pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    pFileDialog->setFileMode(QFileDialog::DirectoryOnly);
#endif
    pFileDialog->setOption(QFileDialog::ShowDirsOnly, true);

    if (pFileDialog->exec() != QDialog::Accepted) {
        QNDEBUG("enex", "Cancelled changing the ENEX file export dir");
        return;
    }

    const auto dirs = pFileDialog->selectedFiles();
    if (dirs.isEmpty()) {
        QNDEBUG("enex", "No directories were selected");
        setStatusText(tr("No directory for ENEX export was selected"));
        return;
    }

    const QString dir = dirs.at(0);

    const QFileInfo dirInfo{dir};
    if (!dirInfo.exists()) {
        QNDEBUG("enex", "Nonexistent directory or file was selected: " << dir);
        setStatusText(tr("No existing directory was selected"));
        return;
    }

    if (!dirInfo.isDir()) {
        QNDEBUG("enex", "Non-directory was selected: " << dir);
        setStatusText(tr("Non-directory was selected"));
        return;
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG("enex", "Non-writable directory was selected: " << dir);
        setStatusText(tr("The selected directory is not writable"));
        return;
    }

    m_ui->folderLineEdit->setText(
        QDir::toNativeSeparators(dirInfo.absoluteFilePath()));

    persistExportFolderSetting();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::onFileNameEdited(const QString & name)
{
    QNDEBUG("enex", "EnexExportDialog::onFileNameEdited: " << name);
    clearAndHideStatus();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::onFolderEdited(const QString & name)
{
    QNDEBUG("enex", "EnexExportDialog::onFolderEdited: " << name);
    clearAndHideStatus();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::createConnections()
{
    QNDEBUG("enex", "EnexExportDialog::createConnections");

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    QObject::connect(
        m_ui->exportTagsCheckBox, &QCheckBox::checkStateChanged, this,
        &EnexExportDialog::onExportTagsOptionChanged);
#else
    QObject::connect(
        m_ui->exportTagsCheckBox, &QCheckBox::stateChanged, this,
        &EnexExportDialog::onExportTagsOptionChanged);
#endif

    QObject::connect(
        m_ui->browseFolderPushButton, &QPushButton::clicked, this,
        &EnexExportDialog::onBrowseFolderButtonPressed);

    QObject::connect(
        m_ui->fileNameLineEdit, &QLineEdit::textEdited, this,
        &EnexExportDialog::onFileNameEdited);

    QObject::connect(
        m_ui->folderLineEdit, &QLineEdit::textEdited, this,
        &EnexExportDialog::onFolderEdited);
}

void EnexExportDialog::persistExportFolderSetting()
{
    const QString path = m_ui->folderLineEdit->text();
    const QString convertedPath = QDir::fromNativeSeparators(path);

    QNDEBUG(
        "enex",
        "EnexExportDialog::persistExportFolderSetting: path = "
            << path << ", converted path: " << convertedPath);

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::auxiliary};

    appSettings.beginGroup(preferences::keys::enexExportImportGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};

    appSettings.setValue(
        preferences::keys::lastExportNotesToEnexPath, convertedPath);
}

void EnexExportDialog::checkConditionsAndEnableDisableOkButton()
{
    QNDEBUG(
        "enex", "EnexExportDialog::checkConditionsAndEnableDisableOkButton");

    const QString fullFilePath = exportEnexFilePath();
    if (!fullFilePath.isEmpty()) {
        QNDEBUG(
            "enex",
            "Full file path is not empty which means it has been "
                << "validated already, enabling the ok button");

        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        clearAndHideStatus();
        return;
    }

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

    const QString fileName = m_ui->fileNameLineEdit->text();
    if (fileName.isEmpty()) {
        QNDEBUG("enex", "The file name is not set");
        return;
    }

    QString folderPath = m_ui->folderLineEdit->text();
    if (folderPath.isEmpty()) {
        QNDEBUG("enex", "The folder is not set");
        return;
    }

    // If both are non-empty, they don't form a valid writable file path
    folderPath = QDir::fromNativeSeparators(folderPath);

    const QFileInfo dirInfo{folderPath};
    if (!dirInfo.exists()) {
        QNDEBUG("enex", "Dir doesn't exist");
        setStatusText(tr("The selected directory doens't exist"));
        return;
    }

    if (!dirInfo.isDir()) {
        QNDEBUG("enex", "Non-dir selected");
        setStatusText(tr("The selected directory path is not a directory"));
        return;
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG("enex", "Non-writable dir selected");
        setStatusText(tr("The selected directory is not writable"));
        return;
    }
}

void EnexExportDialog::setStatusText(const QString & text)
{
    m_ui->statusTextLabel->setText(text);
    m_ui->statusTextLabel->setHidden(false);
}

void EnexExportDialog::clearAndHideStatus()
{
    QNDEBUG("enex", "EnexExportDialog::clearAndHideStatus");

    m_ui->statusTextLabel->setText(QString());
    m_ui->statusTextLabel->setHidden(true);
}

} // namespace quentier

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

#include "EnexExportDialog.h"
#include "ui_EnexExportDialog.h"

#include <lib/preferences/SettingsNames.h>

#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/logging/QuentierLogger.h>

#include <QFileInfo>
#include <QScopedPointer>
#include <QFileDialog>
#include <QDir>

namespace quentier {

EnexExportDialog::EnexExportDialog(
        const Account & account,
        QWidget * parent,
        const QString & suggestedFileName) :
    QDialog(parent),
    m_pUi(new Ui::EnexExportDialog),
    m_currentAccount(account)
{
    m_pUi->setupUi(this);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    QString lastEnexExportPath =
        appSettings.value(LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY).toString();
    bool exportTags =
        (appSettings.contains(LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY)
         ? appSettings.value(LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY).toBool()
         : true);
    appSettings.endGroup();

    if (lastEnexExportPath.isEmpty()) {
        lastEnexExportPath = documentsPath();
    }

    QFileInfo lastEnexExportPathInfo(lastEnexExportPath);
    if (lastEnexExportPathInfo.exists() &&
        lastEnexExportPathInfo.isDir() &&
        lastEnexExportPathInfo.isWritable())
    {
        m_pUi->folderLineEdit->setText(lastEnexExportPath);
        QNDEBUG("EnexExportDialog: initialized folder path to "
                << lastEnexExportPath);
    }

    m_pUi->exportTagsCheckBox->setChecked(exportTags);
    m_pUi->fileNameLineEdit->setText(suggestedFileName);
    m_pUi->statusTextLabel->setHidden(true);

    createConnections();
    checkConditionsAndEnableDisableOkButton();
}

EnexExportDialog::~EnexExportDialog()
{
    delete m_pUi;
}

bool EnexExportDialog::exportTags() const
{
    return m_pUi->exportTagsCheckBox->isChecked();
}

QString EnexExportDialog::exportEnexFilePath() const
{
    QString folderPath = m_pUi->folderLineEdit->text();
    QString convertedFolderPath = QDir::fromNativeSeparators(folderPath);

    QString fileName = m_pUi->fileNameLineEdit->text();

    QNDEBUG("EnexExportDialog::exportEnexFilePath: folder path = "
            << folderPath << ", converted folder path = "
            << convertedFolderPath << ", file name = " << fileName);

    if (convertedFolderPath.isEmpty()) {
        QNDEBUG("No folder path was specified");
        return QString();
    }

    if (fileName.isEmpty()) {
        QNDEBUG("No file name was specified");
        return QString();
    }

    QFileInfo dirInfo(convertedFolderPath);
    if (!dirInfo.exists()) {
        QNDEBUG("Directory doesn't exist");
        return QString();
    }

    if (!dirInfo.isDir()) {
        QNDEBUG("Non-directory was selected");
        return QString();
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG("Non-writable directory was selected");
        return QString();
    }

    if (!convertedFolderPath.endsWith(QStringLiteral("/"))) {
        convertedFolderPath += QStringLiteral("/");
    }

    if (!fileName.endsWith(QStringLiteral(".enex"), Qt::CaseInsensitive)) {
        fileName += QStringLiteral(".enex");
    }

    convertedFolderPath += fileName;
    QNDEBUG("Absolute ENEX export file path: " << convertedFolderPath);
    return convertedFolderPath;
}

void EnexExportDialog::onExportTagsOptionChanged(int state)
{
    QNDEBUG("EnexExportDialog::onExportTagsOptionChanged: state = " << state);

    bool checked = (state == Qt::Checked);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    appSettings.setValue(LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY, checked);
    appSettings.endGroup();

    Q_EMIT exportTagsOptionChanged(checked);
}

void EnexExportDialog::onBrowseFolderButtonPressed()
{
    QNDEBUG("EnexExportDialog::onBrowseFolderButtonPressed");

    clearAndHideStatus();

    QString currentExportDir = m_pUi->folderLineEdit->text();
    if (!currentExportDir.isEmpty())
    {
        QFileInfo currentExportDirInfo(currentExportDir);
        if (!currentExportDirInfo.exists() || !currentExportDirInfo.isDir()) {
            currentExportDir.clear();
        }
    }

    QScopedPointer<QFileDialog> pFileDialog(
        new QFileDialog(this,
                        tr("Please select the folder in which to store "
                           "the output ENEX file"),
                        currentExportDir));
    pFileDialog->setWindowModality(Qt::WindowModal);
    pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    pFileDialog->setFileMode(QFileDialog::DirectoryOnly);
    pFileDialog->setOption(QFileDialog::ShowDirsOnly, true);

    if (pFileDialog->exec() != QDialog::Accepted) {
        QNDEBUG("Cancelled changing the ENEX file export dir");
        return;
    }

    QStringList dirs = pFileDialog->selectedFiles();
    if (dirs.isEmpty()) {
        QNDEBUG("No directories were selected");
        setStatusText(tr("No directory for ENEX export was selected"));
        return;
    }

    QString dir = dirs.at(0);

    QFileInfo dirInfo(dir);
    if (!dirInfo.exists()) {
        QNDEBUG("Nonexistent directory or file was selected: " << dir);
        setStatusText(tr("No existing directory was selected"));
        return;
    }

    if (!dirInfo.isDir()) {
        QNDEBUG("Non-directory was selected: " << dir);
        setStatusText(tr("Non-directory was selected"));
        return;
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG("Non-writable directory was selected: " << dir);
        setStatusText(tr("The selected directory is not writable"));
        return;
    }

    m_pUi->folderLineEdit->setText(
        QDir::toNativeSeparators(dirInfo.absoluteFilePath()));
    persistExportFolderSetting();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::onFileNameEdited(const QString & name)
{
    QNDEBUG("EnexExportDialog::onFileNameEdited: " << name);
    clearAndHideStatus();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::onFolderEdited(const QString & name)
{
    QNDEBUG("EnexExportDialog::onFolderEdited: " << name);
    clearAndHideStatus();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::createConnections()
{
    QNDEBUG("EnexExportDialog::createConnections");

    QObject::connect(m_pUi->exportTagsCheckBox,
                     QNSIGNAL(QCheckBox,stateChanged,int),
                     this,
                     QNSLOT(EnexExportDialog,onExportTagsOptionChanged,int));
    QObject::connect(m_pUi->browseFolderPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(EnexExportDialog,onBrowseFolderButtonPressed));
    QObject::connect(m_pUi->fileNameLineEdit,
                     QNSIGNAL(QLineEdit,textEdited,QString),
                     this,
                     QNSLOT(EnexExportDialog,onFileNameEdited,QString));
    QObject::connect(m_pUi->folderLineEdit,
                     QNSIGNAL(QLineEdit,textEdited,QString),
                     this,
                     QNSLOT(EnexExportDialog,onFolderEdited,QString));
}

void EnexExportDialog::persistExportFolderSetting()
{
    QString path = m_pUi->folderLineEdit->text();
    QString convertedPath = QDir::fromNativeSeparators(path);

    QNDEBUG("EnexExportDialog::persistExportFolderSetting: path = "
            << path << ", converted path: " << convertedPath);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    appSettings.setValue(LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY, convertedPath);
    appSettings.endGroup();
}

void EnexExportDialog::checkConditionsAndEnableDisableOkButton()
{
    QNDEBUG("EnexExportDialog::checkConditionsAndEnableDisableOkButton");

    QString fullFilePath = exportEnexFilePath();
    if (!fullFilePath.isEmpty()) {
        QNDEBUG("Full file path is not empty which means it has been "
                "validated already, enabling the ok button");
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        clearAndHideStatus();
        return;
    }

    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

    QString fileName = m_pUi->fileNameLineEdit->text();
    if (fileName.isEmpty()) {
        QNDEBUG("The file name is not set");
        return;
    }

    QString folderPath = m_pUi->folderLineEdit->text();
    if (folderPath.isEmpty()) {
        QNDEBUG("The folder is not set");
        return;
    }

    // If both are non-empty, they don't form a valid writable file path
    folderPath = QDir::fromNativeSeparators(folderPath);

    QFileInfo dirInfo(folderPath);
    if (!dirInfo.exists()) {
        QNDEBUG("Dir doesn't exist");
        setStatusText(tr("The selected directory doens't exist"));
        return;
    }

    if (!dirInfo.isDir()) {
        QNDEBUG("Non-dir selected");
        setStatusText(tr("The selected directory path is not a directory"));
        return;
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG("Non-writable dir selected");
        setStatusText(tr("The selected directory is not writable"));
        return;
    }
}

void EnexExportDialog::setStatusText(const QString & text)
{
    m_pUi->statusTextLabel->setText(text);
    m_pUi->statusTextLabel->setHidden(false);
}

void EnexExportDialog::clearAndHideStatus()
{
    QNDEBUG("EnexExportDialog::clearAndHideStatus");

    m_pUi->statusTextLabel->setText(QString());
    m_pUi->statusTextLabel->setHidden(true);
}

} // namespace quentier

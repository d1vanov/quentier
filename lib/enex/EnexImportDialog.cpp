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

#include "EnexImportDialog.h"
#include "ui_EnexImportDialog.h"

#include <lib/model/NotebookModel.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Notebook.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>

#include <QStringListModel>
#include <QModelIndex>
#include <QCompleter>
#include <QFileInfo>
#include <QScopedPointer>
#include <QFileDialog>

#include <algorithm>

namespace quentier {

EnexImportDialog::EnexImportDialog(
        const Account & account,
        NotebookModel & notebookModel,
        QWidget *parent) :
    QDialog(parent),
    m_pUi(new Ui::EnexImportDialog),
    m_currentAccount(account),
    m_pNotebookModel(&notebookModel),
    m_pNotebookNamesModel(new QStringListModel(this))
{
    m_pUi->setupUi(this);

    setWindowTitle(tr("Import ENEX"));

    fillNotebookNames();
    m_pUi->notebookNameComboBox->setModel(m_pNotebookNamesModel);

    QCompleter * pCompleter = m_pUi->notebookNameComboBox->completer();
    if (pCompleter) {
        pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    }

    fillDialogContents();
    createConnections();
}

EnexImportDialog::~EnexImportDialog()
{
    delete m_pUi;
}

QString EnexImportDialog::importEnexFilePath(ErrorString * pErrorDescription) const
{
    QNDEBUG("EnexImportDialog::importEnexFilePath");

    QString currentFilePath =
        QDir::fromNativeSeparators(m_pUi->filePathLineEdit->text());
    QNTRACE("Current file path: " << currentFilePath);

    if (currentFilePath.isEmpty()) {
        return QString();
    }

    QFileInfo fileInfo(currentFilePath);
    if (!fileInfo.exists())
    {
        QNDEBUG("ENEX file at specified path doesn't exist");
        if (pErrorDescription) {
            pErrorDescription->setBase(QT_TR_NOOP("ENEX file at specified path "
                                                  "doesn't exist"));
        }

        return QString();
    }

    if (!fileInfo.isFile())
    {
        QNDEBUG("The specified path is not a file");
        if (pErrorDescription) {
            pErrorDescription->setBase(QT_TR_NOOP("The specified path is not "
                                                  "a file"));
        }

        return QString();
    }

    if (!fileInfo.isReadable())
    {
        QNDEBUG("The specified file is not readable");
        if (pErrorDescription) {
            pErrorDescription->setBase(QT_TR_NOOP("The specified file is not "
                                                  "readable"));
        }

        return QString();
    }

    return currentFilePath;
}

QString EnexImportDialog::notebookName(ErrorString * pErrorDescription) const
{
    QString currentNotebookName = m_pUi->notebookNameComboBox->currentText();
    if (currentNotebookName.isEmpty()) {
        return QString();
    }

    if (Notebook::validateName(currentNotebookName, pErrorDescription)) {
        return currentNotebookName;
    }

    return QString();
}

void EnexImportDialog::onBrowsePushButtonClicked()
{
    QNDEBUG("EnexImportDialog::onBrowsePushButtonClicked");

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    QString lastEnexImportPath =
        appSettings.value(LAST_IMPORT_ENEX_PATH_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastEnexImportPath.isEmpty()) {
        lastEnexImportPath = documentsPath();
    }

    QScopedPointer<QFileDialog> pEnexFileDialog(
        new QFileDialog(this,
                        tr("Please select the ENEX file to import"),
                        lastEnexImportPath));
    pEnexFileDialog->setWindowModality(Qt::WindowModal);
    pEnexFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    pEnexFileDialog->setFileMode(QFileDialog::ExistingFile);
    pEnexFileDialog->setDefaultSuffix(QStringLiteral("enex"));

    if (pEnexFileDialog->exec() != QDialog::Accepted) {
        QNDEBUG("The import of ENEX was cancelled");
        return;
    }

    QStringList selectedFiles = pEnexFileDialog->selectedFiles();
    int numSelectedFiles = selectedFiles.size();

    if (numSelectedFiles == 0) {
        QNDEBUG("No ENEX file was selected");
        setStatusText(tr("No ENEX file was selected"));
        return;
    }

    if (numSelectedFiles > 1) {
        QNDEBUG("More than one ENEX files were selected");
        setStatusText(tr("More than one ENEX files were selected"));
        return;
    }

    QFileInfo enexFileInfo(selectedFiles[0]);
    if (!enexFileInfo.exists()) {
        QNDEBUG("The selected ENEX file does not exist");
        setStatusText(tr("The selected ENEX file does not exist"));
        return;
    }

    if (!enexFileInfo.isReadable()) {
        QNDEBUG("The selected ENEX file is not readable");
        setStatusText(tr("The selected ENEX file is not readable"));
        return;
    }

    lastEnexImportPath = pEnexFileDialog->directory().absolutePath();

    if (!lastEnexImportPath.isEmpty()) {
        appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
        appSettings.setValue(LAST_IMPORT_ENEX_PATH_SETTINGS_KEY, lastEnexImportPath);
        appSettings.endGroup();
    }

    m_pUi->filePathLineEdit->setText(
        QDir::toNativeSeparators(enexFileInfo.absoluteFilePath()));
    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::onNotebookNameEdited(const QString & name)
{
    QNDEBUG("EnexImportDialog::onNotebookNameEdited: " << name);
    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::onEnexFilePathEdited(const QString & path)
{
    QNDEBUG("EnexImportDialog::onEnexFilePathEdited: " << path);
    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::dataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QNDEBUG("EnexImportDialog::dataChanged");

    Q_UNUSED(roles)

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        QNDEBUG("At least one of changed indexes is invalid");
        fillNotebookNames();
        return;
    }

    if ((topLeft.column() > NotebookModel::Columns::Name) ||
        (bottomRight.column() < NotebookModel::Columns::Name))
    {
        QNTRACE("The updated indexed don't contain the notebook name");
        return;
    }

    fillNotebookNames();
}

void EnexImportDialog::rowsInserted(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("EnexImportDialog::rowsInserted");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    fillNotebookNames();
}

void EnexImportDialog::rowsAboutToBeRemoved(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("EnexImportDialog::rowsAboutToBeRemoved");

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG("Notebook model is null, nothing to do");
        return;
    }

    QStringList currentNotebookNames = m_pNotebookNamesModel->stringList();

    for(int i = start; i <= end; ++i)
    {
        QModelIndex index =
            m_pNotebookModel->index(i, NotebookModel::Columns::Name, parent);
        QString removedNotebookName = m_pNotebookModel->data(index).toString();
        if (Q_UNLIKELY(removedNotebookName.isEmpty())) {
            continue;
        }

        auto it = std::lower_bound(currentNotebookNames.constBegin(),
                                   currentNotebookNames.constEnd(),
                                   removedNotebookName);
        if ((it != currentNotebookNames.constEnd()) &&
            (*it == removedNotebookName))
        {
            int offset = static_cast<int>(
                std::distance(currentNotebookNames.constBegin(), it));
            QStringList::iterator nit = currentNotebookNames.begin() + offset;
            Q_UNUSED(currentNotebookNames.erase(nit));
        }
    }

    m_pNotebookNamesModel->setStringList(currentNotebookNames);
}

void EnexImportDialog::accept()
{
    QNDEBUG("EnexImportDialog::accept");

    QString notebookName = m_pUi->notebookNameComboBox->currentText();

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    appSettings.setValue(LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY, notebookName);
    appSettings.endGroup();

    QDialog::accept();
}

void EnexImportDialog::createConnections()
{
    QNDEBUG("EnexImportDialog::createConnections");

    QObject::connect(m_pUi->browsePushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(EnexImportDialog,onBrowsePushButtonClicked));
    QObject::connect(m_pUi->filePathLineEdit,
                     QNSIGNAL(QLineEdit,textEdited,QString),
                     this,
                     QNSLOT(EnexImportDialog,onEnexFilePathEdited,QString));
    QObject::connect(m_pUi->notebookNameComboBox,
                     QNSIGNAL(QComboBox,editTextChanged,QString),
                     this,
                     QNSLOT(EnexImportDialog,onNotebookNameEdited,QString));
    QObject::connect(m_pUi->notebookNameComboBox,
                     SIGNAL(currentIndexChanged(QString)),
                     this,
                     SLOT(onNotebookNameEdited(QString)));

    if (!m_pNotebookModel.isNull())
    {
        QObject::connect(m_pNotebookModel.data(), &NotebookModel::dataChanged,
                         this, &EnexImportDialog::dataChanged);
        QObject::connect(m_pNotebookModel.data(),
                         QNSIGNAL(NotebookModel,rowsInserted,QModelIndex,int,int),
                         this,
                         QNSLOT(EnexImportDialog,rowsInserted,QModelIndex,int,int));
        QObject::connect(m_pNotebookModel.data(),
                         QNSIGNAL(NotebookModel,rowsAboutToBeRemoved,
                                  QModelIndex,int,int),
                         this,
                         QNSLOT(EnexImportDialog,rowsAboutToBeRemoved,
                                QModelIndex,int,int));
    }
}

void EnexImportDialog::fillNotebookNames()
{
    QNDEBUG("EnexImportDialog::fillNotebookNames");

    QStringList notebookNames;

    if (!m_pNotebookModel.isNull()) {
        NotebookModel::NotebookFilters filter(
            NotebookModel::NotebookFilter::CanCreateNotes);
        notebookNames = m_pNotebookModel->notebookNames(filter);
    }

    m_pNotebookNamesModel->setStringList(notebookNames);
}

void EnexImportDialog::fillDialogContents()
{
    QNDEBUG("EnexImportDialog::fillDialogContents");

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    QString lastImportEnexNotebookName =
        appSettings.value(LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastImportEnexNotebookName.isEmpty()) {
        lastImportEnexNotebookName = tr("Imported notes");
    }

    QStringList notebookNames = m_pNotebookNamesModel->stringList();
    auto it = std::lower_bound(notebookNames.constBegin(),
                               notebookNames.constEnd(),
                               lastImportEnexNotebookName);
    if ((it != notebookNames.constEnd()) && (*it == lastImportEnexNotebookName))
    {
        int notebookNameIndex =
            static_cast<int>(std::distance(notebookNames.constBegin(), it));
        m_pUi->notebookNameComboBox->setCurrentIndex(notebookNameIndex);
    }
    else
    {
        m_pUi->notebookNameComboBox->setEditText(lastImportEnexNotebookName);
    }

    m_pUi->statusTextLabel->setHidden(true);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
}

void EnexImportDialog::setStatusText(const QString & text)
{
    m_pUi->statusTextLabel->setText(text);
    m_pUi->statusTextLabel->setHidden(false);
}

void EnexImportDialog::clearAndHideStatus()
{
    QNDEBUG("EnexImportDialog::clearAndHideStatus");

    m_pUi->statusTextLabel->setText(QString());
    m_pUi->statusTextLabel->setHidden(true);
}

void EnexImportDialog::checkConditionsAndEnableDisableOkButton()
{
    QNDEBUG("EnexImportDialog::"
            "checkConditionsAndEnableDisableOkButton");

    ErrorString error;
    QString enexFilePath = importEnexFilePath(&error);
    if (enexFilePath.isEmpty()) {
        QNDEBUG("The enex file path is invalid, disabling the ok "
                "button");
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        setStatusText(error.localizedString());
        return;
    }

    QString currentNotebookName = notebookName(&error);
    if (currentNotebookName.isEmpty()) {
        QNDEBUG("Notebook name is not set or is not valid, "
                "disabling the ok button");
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        setStatusText(error.localizedString());
        return;
    }

    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
    clearAndHideStatus();
}

} // namespace quentier

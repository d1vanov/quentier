#include "GenericResourceDisplayWidget.h"
#include "ui_GenericResourceDisplayWidget.h"
#include <client/types/IResource.h>
#include <logging/QuteNoteLogger.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>

namespace qute_note {

GenericResourceDisplayWidget::GenericResourceDisplayWidget(const QIcon & icon, const QString & name,
                                                           const QString & size,
                                                           const QStringList & preferredFileSuffixes,
                                                           const QString & mimeTypeName,
                                                           const IResource & resource,
                                                           QWidget * parent) :
    QWidget(parent),
    m_pUI(new Ui::GenericResourceDisplayWidget),
    m_resource(&resource),
    m_preferredFileSuffixes(preferredFileSuffixes),
    m_mimeTypeName(mimeTypeName)
{
    QNDEBUG("GenericResourceDisplayWidget::GenericResourceDisplayWidget: name = " << name
            << ", size = " << size);
    QNTRACE("Resource: " << resource);

    m_pUI->setupUi(this);

    m_pUI->resourceDisplayNameLabel->setText("<html><head/><body><p><span style=\" font-size:14pt; font-weight:600;\">" +
                                             name + "</span></p></body></head></html>");
    m_pUI->resourceDisplayNameLabel->setTextFormat(Qt::RichText);

    m_pUI->resourceSizeLabel->setText("<html><head/><body><p><span style=\" font-size:12pt; font-weight:600;\">" + size +
                                      "</span></p></body></head></html>");
    m_pUI->resourceSizeLabel->setTextFormat(Qt::RichText);

    m_pUI->resourceIconLabel->setPixmap(icon.pixmap(QSize(32,32)));

    if (!QIcon::hasThemeIcon("document-open")) {
        m_pUI->openResourceButton->setIcon(QIcon(":/generic_resource_icons/png/open_with.png"));
    }

    if (!QIcon::hasThemeIcon("document-save-as")) {
        m_pUI->saveResourceButton->setIcon(QIcon(":/generic_resource_icons/png/save.png"));
    }

    QObject::connect(m_pUI->openResourceButton, SIGNAL(released()), this, SLOT(onOpenWithButtonPressed()));
    QObject::connect(m_pUI->saveResourceButton, SIGNAL(released()), this, SLOT(onSaveAsButtonPressed()));
}

void GenericResourceDisplayWidget::onOpenWithButtonPressed()
{
    // TODO: implement:
    // 1) check if the attachment file already exists in a special directory
    // 2) if not, create it there and after that use QDesktopServices::openUrl(QUrl("file://<file path>")); to open it with some app

    // 3) When the file is being created, consider some non-modal mechanism for progress notification
}

void GenericResourceDisplayWidget::onSaveAsButtonPressed()
{
    QNDEBUG("GenericResourceDisplayWidget::onSaveAsButtonPressed");

    if (!m_resource) {
        QNFATAL("Can't save resource: internal pointer to resource is null");
        // TODO: probably also need to use message box to notify user of internal error
        return;
    }

    if (!m_resource->hasDataBody() && !m_resource->hasAlternateDataBody()) {
        QNWARNING("Can't save resource: no data body or alternate data body within the resource");
        // TODO: probably also need to use message box to notify user of internal error
        return;
    }

    const QByteArray & data = (m_resource->hasDataBody()
                               ? m_resource->dataBody()
                               : m_resource->alternateDataBody());

    QString preferredSuffix;
    QString preferredDirectory;

    if (!m_preferredFileSuffixes.isEmpty())
    {
        QSettings settings;
        QStringList childGroups = settings.childGroups();
        int attachmentsSaveLocGroupIndex = childGroups.indexOf("AttachmentSaveLocations");
        if (attachmentsSaveLocGroupIndex >= 0)
        {
            QNTRACE("Found cached attachment save location group within application settings");

            settings.beginGroup("AttachmentSaveLocations");
            QStringList cachedFileSuffixes = settings.childKeys();
            const int numPreferredSuffixes = m_preferredFileSuffixes.size();
            for(int i = 0; i < numPreferredSuffixes; ++i)
            {
                preferredSuffix = m_preferredFileSuffixes[i];
                int indexInCache = cachedFileSuffixes.indexOf(preferredSuffix);
                if (indexInCache < 0) {
                    QNTRACE("Haven't found cached attachment save directory for file suffix " << preferredSuffix);
                    continue;
                }

                QVariant dirValue = settings.value(preferredSuffix);
                if (dirValue.isNull() || !dirValue.isValid()) {
                    QNTRACE("Found inappropriate attachment save directory for file suffix " << preferredSuffix);
                    continue;
                }

                QFileInfo dirInfo(dirValue.toString());
                if (!dirInfo.exists()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " does not exist: " << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isDir()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " is not a directory: " << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isWritable()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " is not writable: " << dirInfo.absolutePath());
                    continue;
                }

                preferredDirectory = dirInfo.absolutePath();
                break;
            }

            settings.endGroup();
        }
    }

    QString filter = m_mimeTypeName + " (*.";
    filter += preferredSuffix;
    filter += " ";

    const int numPreferredSuffixes = m_preferredFileSuffixes.size();
    for(int i = 0; i < numPreferredSuffixes; ++i)
    {
        const QString & currentSuffix = m_preferredFileSuffixes[i];
        if (currentSuffix == preferredSuffix) {
            continue;
        }

        filter += "*.";
        filter += currentSuffix;
        filter += " ";
    }

    // Remove the trailing whitespace
    filter = filter.remove(filter.size()-1, 1);
    filter += ");;";
    filter += QObject::tr("All files");
    filter += " (*)";


    QString fileName = QFileDialog::getSaveFileName(this, QObject::tr("Save as..."),
                                                    preferredDirectory, filter);
    if (fileName.isEmpty()) {
        QNINFO("User cancelled writing to file");
        return;
    }

    bool foundSuffix = false;
    for(int i = 0; i < numPreferredSuffixes; ++i)
    {
        const QString & currentSuffix = m_preferredFileSuffixes[i];
        if (fileName.endsWith(currentSuffix, Qt::CaseInsensitive)) {
            foundSuffix = true;
            break;
        }
    }

    if (!foundSuffix) {
        fileName += "." + preferredSuffix;
    }

    // TODO: consider moving the file write operation to a separate thread to not block the GUI thread

    QFile file(preferredDirectory + "/" + fileName);
    bool open = file.open(QIODevice::WriteOnly);
    if (open)
    {
        qint64 writtenBytes = file.write(data);
        if (writtenBytes < data.length()) {
            QNFATAL("Could not write resource to file properly: the number of bytes "
                    "written is less than the size of the data");
            // TODO: probably also need to use message box to notify user of internal error
        }
    }
}

} // namespace qute_note

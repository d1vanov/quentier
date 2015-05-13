#include "EncryptedAreaPlugin.h"
#include "ui_EncryptedAreaPlugin.h"
#include <logging/QuteNoteLogger.h>
#include <client/types/IResource.h>
#include <QIcon>
#include <QMouseEvent>

namespace qute_note {

EncryptedAreaPlugin::EncryptedAreaPlugin(QWidget * parent) :
    INoteEditorPlugin(parent),
    m_pUI(new Ui::EncryptedAreaPlugin)
{
    m_pUI->setupUi(this);

    if (!QIcon::hasThemeIcon("security-high")) {
        QIcon lockIcon;
        lockIcon.addFile(":/encrypted_area_icons/png/lock-16x16", QSize(16, 16));
        lockIcon.addFile(":/encrypted_area_icons/png/lock-24x24", QSize(24, 24));
        lockIcon.addFile(":/encrypted_area_icons/png/lock-32x32", QSize(32, 32));
        lockIcon.addFile(":/encrypted_area_icons/png/lock-48x48", QSize(48, 48));
        m_pUI->iconPushButton->setIcon(lockIcon);
    }
}

EncryptedAreaPlugin::~EncryptedAreaPlugin()
{
    delete m_pUI;
}

EncryptedAreaPlugin * EncryptedAreaPlugin::clone() const
{
    return new EncryptedAreaPlugin();
}

bool EncryptedAreaPlugin::initialize(const QString & mimeType, const QUrl & url,
                                     const QStringList & parameterNames,
                                     const QStringList & parameterValues,
                                     const IResource * resource, QString & errorDescription)
{
    QNDEBUG("EncryptedAreaPlugin::initialize: mime type = " << mimeType
            << ", url = " << url.toString() << ", parameter names = " << parameterNames.join(", ")
            << ", parameter values = " << parameterValues.join(", "));

    Q_UNUSED(resource);

    // TODO: implement
    Q_UNUSED(errorDescription);
    return true;
}

QStringList EncryptedAreaPlugin::mimeTypes() const
{
    return QStringList();
}

QHash<QString, QStringList> EncryptedAreaPlugin::fileExtensions() const
{
    return QHash<QString, QStringList>();
}

QStringList EncryptedAreaPlugin::specificAttributes() const
{
    return QStringList() << "en-crypt";
}

QString EncryptedAreaPlugin::name() const
{
    return "EncryptedAreaPlugin";
}

QString EncryptedAreaPlugin::description() const
{
    return QObject::tr("Encrypted area plugin - note editor plugin used for the display and convenient work "
                       "with encrypted text within notes");
}

void EncryptedAreaPlugin::mouseReleaseEvent(QMouseEvent * mouseEvent)
{
    QWidget::mouseReleaseEvent(mouseEvent);

    if (!mouseEvent) {
        return;
    }

    const QPoint & pos = mouseEvent->pos();
    QWidget * child = childAt(pos);
    if (!child) {
        return;
    }

    if (child == m_pUI->iconPushButton) {
        // TODO: raise dialog box to decrypt the text
    }
}

} // namespace qute_note

#include "GenericResourceDisplayWidget.h"
#include <client/types/IResource.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {

GenericResourceDisplayWidget::GenericResourceDisplayWidget(const QIcon & icon, const QString & name,
                                                           const QString & size, const IResource & resource,
                                                           QWidget * parent) :
    QWidget(parent),
    m_resource(nullptr)
{
    QNDEBUG("GenericResourceDisplayWidget::GenericResourceDisplayWidget: name = " << name
            << ", size = " << size);
    QNTRACE("Resource: " << resource);

    m_resource = &resource;

    // TODO: compose the widget
    // TODO: put the icon on the widget
    // TODO: put the resource name and size on the widget
    // TODO: put "open with" link on the widget
    // TODO: put "save as" link on the widget
    Q_UNUSED(icon);
}

} // namespace qute_note

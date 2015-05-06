#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <QWidget>
#include <QCache>

QT_FORWARD_DECLARE_CLASS(QMimeDatabase)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(const IResource & resource, const QMimeDatabase & mimeDatabase,
                                 QCache<QString, QIcon> & iconCache, QWidget * parent = nullptr);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

private:
    QIcon getIconForMimeType(const QString & mimeTypeName, const QMimeDatabase & mimeDatabase) const;

private:
    const IResource *     m_resource;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include "INoteEditorPlugin.h"

namespace qute_note {

INoteEditorPlugin::INoteEditorPlugin(QWidget * parent) :
    QWidget(parent)
{}

QList<QPair<QString, QString> > INoteEditorPlugin::specificParameters() const
{
    return QList<QPair<QString, QString> >();
}

} // namespace qute_note

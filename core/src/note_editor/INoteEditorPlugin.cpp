#include "INoteEditorPlugin.h"

namespace qute_note {

INoteEditorPlugin::INoteEditorPlugin(QWidget * parent) :
    QWidget(parent)
{}

QStringList INoteEditorPlugin::specificAttributes() const
{
    return QStringList();
}

} // namespace qute_note

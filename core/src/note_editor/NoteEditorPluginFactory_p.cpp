#include "NoteEditorPluginFactory_p.h"

namespace qute_note {

NoteEditorPluginFactoryPrivate::NoteEditorPluginFactoryPrivate()
{}

NoteEditorPluginFactoryPrivate::PluginIdentifier NoteEditorPluginFactoryPrivate::addPlugin(INoteEditorPlugin * plugin,
                                                                                           QString & errorDescription,
                                                                                           const bool forceOverrideTypeKeys)
{
    // TODO: implement
    Q_UNUSED(plugin)
    Q_UNUSED(errorDescription)
    Q_UNUSED(forceOverrideTypeKeys)
    return 0;
}

bool NoteEditorPluginFactoryPrivate::removePlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id,
                                                  QString & errorDescription)
{
    // TODO: implement
    Q_UNUSED(id)
    Q_UNUSED(errorDescription)
    return true;
}

bool NoteEditorPluginFactoryPrivate::hasPlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id) const
{
    // TODO: implement
    Q_UNUSED(id)
    return true;
}

QObject * NoteEditorPluginFactoryPrivate::create(const QString & mimeType, const QUrl & url,
                                                 const QStringList & argumentNames,
                                                 const QStringList & argumentValues) const
{
    // TODO: implement
    Q_UNUSED(mimeType)
    Q_UNUSED(url)
    Q_UNUSED(argumentNames)
    Q_UNUSED(argumentValues)
    return nullptr;
}

QList<QWebPluginFactory::Plugin> NoteEditorPluginFactoryPrivate::plugins() const
{
    // TODO: implement
    return QList<QWebPluginFactory::Plugin>();
}

} // namespace qute_note

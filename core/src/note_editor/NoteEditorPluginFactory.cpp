#include "NoteEditorPluginFactory.h"
#include "NoteEditorPluginFactory_p.h"

namespace qute_note {

NoteEditorPluginFactory::NoteEditorPluginFactory(QObject * parent) :
    QWebPluginFactory(parent),
    d_ptr(new NoteEditorPluginFactoryPrivate(*this, this))
{}

NoteEditorPluginFactory::~NoteEditorPluginFactory()
{}

NoteEditorPluginFactory::PluginIdentifier NoteEditorPluginFactory::addPlugin(INoteEditorPlugin * plugin,
                                                                             QString & errorDescription,
                                                                             const bool forceOverrideTypeKeys)
{
    Q_D(NoteEditorPluginFactory);
    return d->addPlugin(plugin, errorDescription, forceOverrideTypeKeys);
}

bool NoteEditorPluginFactory::removePlugin(const NoteEditorPluginFactory::PluginIdentifier id,
                                           QString & errorDescription)
{
    Q_D(NoteEditorPluginFactory);
    return d->removePlugin(id, errorDescription);
}

bool NoteEditorPluginFactory::hasPlugin(const NoteEditorPluginFactory::PluginIdentifier id) const
{
    Q_D(const NoteEditorPluginFactory);
    return d->hasPlugin(id);
}

void NoteEditorPluginFactory::setFallbackResourceIcon(const QIcon & icon)
{
    Q_D(NoteEditorPluginFactory);
    d->setFallbackResourceIcon(icon);
}

QObject * NoteEditorPluginFactory::create(const QString & mimeType, const QUrl & url,
                                          const QStringList & argumentNames,
                                          const QStringList & argumentValues) const
{
    Q_D(const NoteEditorPluginFactory);
    return d->create(mimeType, url, argumentNames, argumentValues);
}

QList<QWebPluginFactory::Plugin> NoteEditorPluginFactory::plugins() const
{
    Q_D(const NoteEditorPluginFactory);
    return d->plugins();
}

} // namespace qute_note

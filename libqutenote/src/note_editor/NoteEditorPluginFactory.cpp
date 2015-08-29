#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#include "NoteEditorPluginFactory_p.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/note_editor/IGenericResourceDisplayWidget.h>
#include <QRegExp>

namespace qute_note {

NoteEditorPluginFactory::NoteEditorPluginFactory(const NoteEditor & noteEditor,
                                                 const ResourceFileStorageManager & resourceFileStorageManager,
                                                 const FileIOThreadWorker & fileIOThreadWorker,
                                                 QObject * parent) :
    QWebPluginFactory(parent),
    d_ptr(new NoteEditorPluginFactoryPrivate(*this, noteEditor, resourceFileStorageManager, fileIOThreadWorker, this))
{}

NoteEditorPluginFactory::~NoteEditorPluginFactory()
{}

const NoteEditor & NoteEditorPluginFactory::noteEditor() const
{
    Q_D(const NoteEditorPluginFactory);
    return d->noteEditor();
}

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

void NoteEditorPluginFactory::setGenericResourceDisplayWidget(IGenericResourceDisplayWidget * genericResourceDisplayWidget)
{
    Q_D(NoteEditorPluginFactory);
    d->setGenericResourceDisplayWidget(genericResourceDisplayWidget);
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

bool NoteEditorPluginFactory::hasPluginForMimeType(const QString & mimeType) const
{
    Q_D(const NoteEditorPluginFactory);
    return d->hasPluginForMimeType(mimeType);
}

bool NoteEditorPluginFactory::hasPluginForMimeType(const QRegExp & mimeTypeRegex) const
{
    Q_D(const NoteEditorPluginFactory);
    return d->hasPluginForMimeType(mimeTypeRegex);
}

void NoteEditorPluginFactory::setNote(const Note & note)
{
    Q_D(NoteEditorPluginFactory);
    return d->setNote(note);
}

} // namespace qute_note

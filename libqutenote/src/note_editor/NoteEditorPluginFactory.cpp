#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#include "NoteEditorPluginFactory_p.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/note_editor/IGenericResourceDisplayWidget.h>
#include <QRegExp>

namespace qute_note {

NoteEditorPluginFactory::NoteEditorPluginFactory(const NoteEditor & noteEditor,
                                                 const ResourceFileStorageManager & resourceFileStorageManager,
                                                 const FileIOThreadWorker & fileIOThreadWorker,
                                                 INoteEditorEncryptedAreaPlugin * pEncryptedAreaPlugin,
                                                 QObject * parent) :
    QWebPluginFactory(parent),
    d_ptr(new NoteEditorPluginFactoryPrivate(*this, noteEditor, resourceFileStorageManager, fileIOThreadWorker, pEncryptedAreaPlugin, this))
{}

NoteEditorPluginFactory::~NoteEditorPluginFactory()
{}

const NoteEditor & NoteEditorPluginFactory::noteEditor() const
{
    Q_D(const NoteEditorPluginFactory);
    return d->noteEditor();
}

NoteEditorPluginFactory::ResourcePluginIdentifier NoteEditorPluginFactory::addResourcePlugin(INoteEditorResourcePlugin * plugin,
                                                                                             QString & errorDescription,
                                                                                             const bool forceOverrideTypeKeys)
{
    Q_D(NoteEditorPluginFactory);
    return d->addResourcePlugin(plugin, errorDescription, forceOverrideTypeKeys);
}

bool NoteEditorPluginFactory::removeResourcePlugin(const NoteEditorPluginFactory::ResourcePluginIdentifier id,
                                           QString & errorDescription)
{
    Q_D(NoteEditorPluginFactory);
    return d->removeResourcePlugin(id, errorDescription);
}

bool NoteEditorPluginFactory::hasResourcePlugin(const NoteEditorPluginFactory::ResourcePluginIdentifier id) const
{
    Q_D(const NoteEditorPluginFactory);
    return d->hasResourcePlugin(id);
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

void NoteEditorPluginFactory::setEncryptedAreaPlugin(INoteEditorEncryptedAreaPlugin * encryptedAreaPlugin)
{
    Q_D(NoteEditorPluginFactory);
    d->setEncryptedAreaPlugin(encryptedAreaPlugin);
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

bool NoteEditorPluginFactory::hasResourcePluginForMimeType(const QString & mimeType) const
{
    Q_D(const NoteEditorPluginFactory);
    return d->hasResourcePluginForMimeType(mimeType);
}

bool NoteEditorPluginFactory::hasResourcePluginForMimeType(const QRegExp & mimeTypeRegex) const
{
    Q_D(const NoteEditorPluginFactory);
    return d->hasResourcePluginForMimeType(mimeTypeRegex);
}

void NoteEditorPluginFactory::setNote(const Note & note)
{
    Q_D(NoteEditorPluginFactory);
    return d->setNote(note);
}

} // namespace qute_note

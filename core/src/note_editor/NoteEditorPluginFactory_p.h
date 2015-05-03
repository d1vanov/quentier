#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H

#include "NoteEditorPluginFactory.h"

namespace qute_note {

class NoteEditorPluginFactoryPrivate
{
public:
    NoteEditorPluginFactoryPrivate();

    typedef NoteEditorPluginFactory::PluginIdentifier PluginIdentifier;

    PluginIdentifier addPlugin(INoteEditorPlugin * plugin, QString & errorDescription,
                               const bool forceOverrideTypeKeys);

    bool removePlugin(const PluginIdentifier id, QString & errorDescription);

    bool hasPlugin(const PluginIdentifier id) const;

    QObject * create(const QString & mimeType, const QUrl & url,
                     const QStringList & argumentNames,
                     const QStringList & argumentValues) const;

    QList<QWebPluginFactory::Plugin> plugins() const;
};

} // namespace qute_note;

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H

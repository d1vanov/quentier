#include "NoteEditorPluginFactory_p.h"
#include "GenericResourceDisplayWidget.h"
#include <logging/QuteNoteLogger.h>

namespace qute_note {

NoteEditorPluginFactoryPrivate::NoteEditorPluginFactoryPrivate(NoteEditorPluginFactory & factory,
                                                               QObject * parent) :
    QObject(parent),
    m_plugins(),
    m_lastFreePluginId(1),
    q_ptr(&factory)
{}

NoteEditorPluginFactoryPrivate::~NoteEditorPluginFactoryPrivate()
{
    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        INoteEditorPlugin * plugin = it.value();
        delete plugin;
    }
}

NoteEditorPluginFactoryPrivate::PluginIdentifier NoteEditorPluginFactoryPrivate::addPlugin(INoteEditorPlugin * plugin,
                                                                                           QString & errorDescription,
                                                                                           const bool forceOverrideTypeKeys)
{
    if (!plugin) {
        errorDescription = QT_TR_NOOP("Detected attempt to install null note editor plugin");
        QNWARNING(errorDescription);
        return 0;
    }

    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        const INoteEditorPlugin * currentPlugin = it.value();
        if (plugin == currentPlugin) {
            errorDescription = QT_TR_NOOP("Detected attempt to install the same plugin instance more than once");
            QNWARNING(errorDescription);
            return 0;
        }
    }

    const QStringList & mimeTypes = plugin->mimeTypes();
    if (mimeTypes.isEmpty()) {
        errorDescription = QT_TR_NOOP("Can't install note editor plugin without specified mime types");
        QNWARNING(errorDescription);
        return 0;
    }

    if (!forceOverrideTypeKeys)
    {
        const int numMimeTypes = mimeTypes.size();

        for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
        {
            const INoteEditorPlugin * currentPlugin = it.value();
            const QStringList & currentPluginMimeTypes = currentPlugin->mimeTypes();

            for(int i = 0; i < numMimeTypes; ++i)
            {
                const QString & mimeType = mimeTypes[i];
                if (currentPluginMimeTypes.contains(mimeType)) {
                    errorDescription = QT_TR_NOOP("Can't install note editor plugin: found "
                                                  "conflicting mime type " + mimeType +
                                                  " from plugin " + currentPlugin->name());
                    QNINFO(errorDescription);
                    return 0;
                }
            }
        }
    }

    plugin->setParent(nullptr);
    m_plugins[m_lastFreePluginId] = plugin;
    return m_lastFreePluginId++;
}

bool NoteEditorPluginFactoryPrivate::removePlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id,
                                                  QString & errorDescription)
{
    auto it = m_plugins.find(id);
    if (it == m_plugins.end()) {
        errorDescription = QT_TR_NOOP("Can't uninstall note editor plugin: plugin with id " +
                                      QString::number(id) + " was not found");
        QNDEBUG(errorDescription);
        return false;
    }

    INoteEditorPlugin * plugin = it.value();
    delete plugin;

    Q_UNUSED(m_plugins.erase(it));

    Q_Q(NoteEditorPluginFactory);
    q->refreshPlugins();

    return true;
}

bool NoteEditorPluginFactoryPrivate::hasPlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id) const
{
    auto it = m_plugins.find(id);
    return (it != m_plugins.end());
}

QObject * NoteEditorPluginFactoryPrivate::create(const QString & mimeType, const QUrl & url,
                                                 const QStringList & argumentNames,
                                                 const QStringList & argumentValues) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::create: mimeType = " << mimeType
            << ", url = " << url.toString() << ", argument names: " << argumentNames.join(", ")
            << ", argument values: " << argumentValues.join(", "));

    if (m_plugins.isEmpty()) {
        // TODO: return generic resource display widget
        return nullptr;
    }

    // Need to loop through installed plugins considering the last installed plugins first
    // Sadly, Qt doesn't support proper reverse iterators for its own containers without STL compatibility so will emulate them
    auto pluginsBegin = m_plugins.begin();
    auto pluginsLast = m_plugins.end();
    --pluginsLast;
    for(auto it = pluginsLast; it != pluginsBegin; --it)
    {
        const INoteEditorPlugin * plugin = it.value();

        const QStringList mimeTypes = plugin->mimeTypes();
        if (mimeTypes.contains(mimeType)) {
            QNTRACE("Will use plugin " << plugin->name());
            return plugin->clone();
        }
    }

    QNTRACE("Haven't found any installed plugin supporting mime type " << mimeType
            << ", will use generic resource display plugin for that");
    // TODO: return generic resource display widget
    return nullptr;
}

QList<QWebPluginFactory::Plugin> NoteEditorPluginFactoryPrivate::plugins() const
{
    QList<QWebPluginFactory::Plugin> plugins;

    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        const INoteEditorPlugin & currentPlugin = *(it.value());

        plugins.push_back(QWebPluginFactory::Plugin());
        auto & plugin = plugins.back();

        plugin.name = currentPlugin.name();
        plugin.description = currentPlugin.description();

        auto & mimeTypes = plugin.mimeTypes;
        const QStringList & currentPluginMimeTypes = currentPlugin.mimeTypes();
        const auto & currentPluginFileExtensions = currentPlugin.fileExtensions();

        const int numMimeTypes = currentPluginMimeTypes.size();
        for(int i = 0; i < numMimeTypes; ++i)
        {
            mimeTypes.push_back(QWebPluginFactory::MimeType());
            auto & mimeType = mimeTypes.back();

            mimeType.name = currentPluginMimeTypes[i];
            auto fileExtIter = currentPluginFileExtensions.find(mimeType.name);
            if (fileExtIter != currentPluginFileExtensions.end()) {
                mimeType.fileExtensions = fileExtIter.value();
            }
        }
    }

    return plugins;
}

} // namespace qute_note

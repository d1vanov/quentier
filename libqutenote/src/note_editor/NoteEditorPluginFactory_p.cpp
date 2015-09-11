#include "NoteEditorPluginFactory_p.h"
#include "GenericResourceDisplayWidget.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/ResourceAdapter.h>
#include <QFileIconProvider>
#include <QDir>
#include <QRegExp>

namespace qute_note {

NoteEditorPluginFactoryPrivate::NoteEditorPluginFactoryPrivate(NoteEditorPluginFactory & factory,
                                                               const NoteEditor & noteEditor,
                                                               const ResourceFileStorageManager & resourceFileStorageManager,
                                                               const FileIOThreadWorker & fileIOThreadWorker,
                                                               QObject * parent) :
    QObject(parent),
    m_noteEditor(noteEditor),
    m_genericResourceDisplayWidget(new GenericResourceDisplayWidget),
    m_plugins(),
    m_lastFreePluginId(1),
    m_pCurrentNote(nullptr),
    m_fallbackResourceIcon(QIcon::fromTheme("unknown")),
    m_mimeDatabase(),
    m_pResourceFileStorageManager(&resourceFileStorageManager),
    m_pFileIOThreadWorker(&fileIOThreadWorker),
    m_resourceIconCache(),
    m_fileSuffixesCache(),
    m_filterStringsCache(),
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

    delete m_genericResourceDisplayWidget;
}

const NoteEditor & NoteEditorPluginFactoryPrivate::noteEditor() const
{
    return m_noteEditor;
}

NoteEditorPluginFactoryPrivate::PluginIdentifier NoteEditorPluginFactoryPrivate::addPlugin(INoteEditorPlugin * plugin,
                                                                                           QString & errorDescription,
                                                                                           const bool forceOverrideTypeKeys)
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::addPlugin: " << (plugin ? plugin->name() : QString("<null>"))
            << ", force override type keys = " << (forceOverrideTypeKeys ? "true" : "false"));

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

    const QStringList mimeTypes = plugin->mimeTypes();
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
            const QStringList currentPluginMimeTypes = currentPlugin->mimeTypes();

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

    PluginIdentifier pluginId = m_lastFreePluginId;
    ++m_lastFreePluginId;

    m_plugins[pluginId] = plugin;

    QNTRACE("Assigned id " << pluginId << " to plugin " << plugin->name());
    return pluginId;
}

bool NoteEditorPluginFactoryPrivate::removePlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id,
                                                  QString & errorDescription)
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::removePlugin: " << id);

    auto it = m_plugins.find(id);
    if (it == m_plugins.end()) {
        errorDescription = QT_TR_NOOP("Can't uninstall note editor plugin: plugin with id " +
                                      QString::number(id) + " was not found");
        QNDEBUG(errorDescription);
        return false;
    }

    INoteEditorPlugin * plugin = it.value();
    QString pluginName = (plugin ? plugin->name() : QString("<null>"));
    QNTRACE("Plugin to remove: " << pluginName);

    delete plugin;
    plugin = nullptr;

    Q_UNUSED(m_plugins.erase(it));

    Q_Q(NoteEditorPluginFactory);
    q->refreshPlugins();

    QNTRACE("Done removing plugin " << id << " (" << pluginName << ")");
    return true;
}

bool NoteEditorPluginFactoryPrivate::hasPlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id) const
{
    auto it = m_plugins.find(id);
    return (it != m_plugins.end());
}

bool NoteEditorPluginFactoryPrivate::hasPluginForMimeType(const QString & mimeType) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::hasPluginForMimeType: " << mimeType);

    if (m_plugins.empty()) {
        return false;
    }

    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        const INoteEditorPlugin * plugin = it.value();
        const QStringList & mimeTypes = plugin->mimeTypes();
        if (mimeTypes.contains(mimeType)) {
            return true;
        }
    }

    return false;
}

bool NoteEditorPluginFactoryPrivate::hasPluginForMimeType(const QRegExp & mimeTypeRegex) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::hasPluginForMimeType: " << mimeTypeRegex.pattern());

    if (m_plugins.empty()) {
        return false;
    }

    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        const INoteEditorPlugin * plugin = it.value();
        const QStringList & mimeTypes = plugin->mimeTypes();
        if (!mimeTypes.filter(mimeTypeRegex).isEmpty()) {
            return true;
        }
    }

    return false;
}

void NoteEditorPluginFactoryPrivate::setNote(const Note & note)
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::setNote: change current note to " << (note.hasTitle() ? note.title() : note.ToQString()));
    m_pCurrentNote = &note;
}

void NoteEditorPluginFactoryPrivate::setFallbackResourceIcon(const QIcon & icon)
{
    m_fallbackResourceIcon = icon;
}

void NoteEditorPluginFactoryPrivate::setGenericResourceDisplayWidget(IGenericResourceDisplayWidget * genericResourceDisplayWidget)
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::setGenericResourceDisplayWidget: "
            << (genericResourceDisplayWidget ? genericResourceDisplayWidget->objectName() : QString("<null>")));

    if (!genericResourceDisplayWidget) {
        QNWARNING("detected attempt to set null generic resource display widget to note editor plugin factory");
        return;
    }

    delete m_genericResourceDisplayWidget;
    m_genericResourceDisplayWidget = genericResourceDisplayWidget;
    m_genericResourceDisplayWidget->setParent(nullptr);
}

QObject * NoteEditorPluginFactoryPrivate::create(const QString & mimeType, const QUrl & url,
                                                 const QStringList & argumentNames,
                                                 const QStringList & argumentValues) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::create: mimeType = " << mimeType
            << ", url = " << url.toString() << ", argument names: " << argumentNames.join(", ")
            << ", argument values: " << argumentValues.join(", "));

    if (!m_pCurrentNote) {
        QNFATAL("Can't create note editor plugin for resource display: no note specified");
        return nullptr;
    }

    QList<ResourceAdapter> resourceAdapters = m_pCurrentNote->resourceAdapters();

    const ResourceAdapter * pCurrentResource = nullptr;
    QString resourceHash;

    int resourceHashIndex = argumentNames.indexOf("hash");
    if (resourceHashIndex >= 0)
    {
        resourceHash = argumentValues.at(resourceHashIndex);
        const int numResources = resourceAdapters.size();
        for(int i = 0; i < numResources; ++i)
        {
            const ResourceAdapter & resource = resourceAdapters[i];
            if (!resource.hasDataHash()) {
                continue;
            }

            if (resource.dataHash() == resourceHash) {
                pCurrentResource = &resource;
                break;
            }
        }
    }

    QNTRACE("Number of installed plugins: " << m_plugins.size());

    if (!m_plugins.isEmpty())
    {
        Q_Q(const NoteEditorPluginFactory);

        // Need to loop through installed mime type-based plugins considering the last installed plugins first
        // Sadly, Qt doesn't support proper reverse iterators for its own containers without STL compatibility so will emulate them
        auto pluginsBegin = m_plugins.begin();
        auto pluginsBeforeBegin = pluginsBegin;
        --pluginsBeforeBegin;

        auto pluginsLast = m_plugins.end();
        --pluginsLast;

        for(auto it = pluginsLast; it != pluginsBeforeBegin; --it)
        {
            const INoteEditorPlugin * plugin = it.value();

            const QStringList mimeTypes = plugin->mimeTypes();
            QNTRACE("Testing plugin " << plugin->name() << " with id " << it.key() << ", mime types: " << mimeTypes.join("; "));

            if (mimeTypes.contains(mimeType))
            {
                QNTRACE("Will use plugin " << plugin->name());
                INoteEditorPlugin * newPlugin = plugin->clone();
                QString errorDescription;
                bool res = newPlugin->initialize(mimeType, url, argumentNames, argumentValues, *q,
                                                 pCurrentResource, errorDescription);
                if (!res) {
                    QNINFO("Can't initialize note editor plugin " << plugin->name() << ": " << errorDescription);
                    delete newPlugin;
                    continue;
                }

                return newPlugin;
            }
        }
    }

    QNTRACE("Haven't found any installed plugin supporting mime type " << mimeType
            << ", will use generic resource display plugin for that");

    if (!pCurrentResource) {
        QNFATAL("Can't find resource in note by data hash: " << resourceHash
                << ", note: " << *m_pCurrentNote);
        return nullptr;
    }

    QString resourceDisplayName;
    if (pCurrentResource->hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = pCurrentResource->resourceAttributes();
        if (attributes.fileName.isSet()) {
            resourceDisplayName = attributes.fileName;
        }
        else if (attributes.sourceURL.isSet()) {
            resourceDisplayName = attributes.sourceURL;
        }
    }

    QString resourceDataSize;
    if (pCurrentResource->hasDataBody()) {
        const QByteArray & data = pCurrentResource->dataBody();
        int bytes = data.size();
        resourceDataSize = humanReadableSize(bytes);
    }
    else if (pCurrentResource->hasAlternateDataBody()) {
        const QByteArray & data = pCurrentResource->alternateDataBody();
        int bytes = data.size();
        resourceDataSize = humanReadableSize(bytes);
    }

    auto cachedIconIt = m_resourceIconCache.find(mimeType);
    if (cachedIconIt == m_resourceIconCache.end()) {
        QIcon resourceIcon = getIconForMimeType(mimeType);
        cachedIconIt = m_resourceIconCache.insert(mimeType, resourceIcon);
    }

    QStringList fileSuffixes;
    auto fileSuffixesIt = m_fileSuffixesCache.find(mimeType);
    if (fileSuffixesIt == m_fileSuffixesCache.end()) {
        fileSuffixes = getFileSuffixesForMimeType(mimeType);
        m_fileSuffixesCache[mimeType] = fileSuffixes;
    }

    QString filterString;
    auto filterStringIt = m_filterStringsCache.find(mimeType);
    if (filterStringIt == m_filterStringsCache.end()) {
        filterString = getFilterStringForMimeType(mimeType);
        m_filterStringsCache[mimeType] = filterString;
    }

    IGenericResourceDisplayWidget * genericResourceDisplayWidget = m_genericResourceDisplayWidget->create();
    QUTE_NOTE_CHECK_PTR(genericResourceDisplayWidget, "null pointer to generic resource display widget");

    // NOTE: upon return this generic resource display widget would be reparented to the caller anyway,
    // the parent setting below is strictly for possible use within initialize method (for example, if
    // the widget would need to create some dialog window, it could be modal due to the existence of the parent)
    genericResourceDisplayWidget->setParent(qobject_cast<QWidget*>(parent()));

    genericResourceDisplayWidget->initialize(cachedIconIt.value(), resourceDisplayName,
                                             resourceDataSize, fileSuffixes, filterString,
                                             *pCurrentResource, *m_pResourceFileStorageManager,
                                             *m_pFileIOThreadWorker);
    return genericResourceDisplayWidget;
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
        mimeTypes.reserve(std::max(numMimeTypes, 0));
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

QIcon NoteEditorPluginFactoryPrivate::getIconForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::getIconForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName
                << ", will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    QString iconName = mimeType.iconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found icon from theme, name = " << iconName);
        return QIcon::fromTheme(iconName, m_fallbackResourceIcon);
    }

    iconName = mimeType.genericIconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found generic icon from theme, name = " << iconName);
        return QIcon::fromTheme(iconName, m_fallbackResourceIcon);
    }

    QStringList suffixes;
    auto fileSuffixesIt = m_fileSuffixesCache.find(mimeTypeName);
    if (fileSuffixesIt == m_fileSuffixesCache.end()) {
        suffixes = getFileSuffixesForMimeType(mimeTypeName);
        m_fileSuffixesCache[mimeTypeName] = suffixes;
    }
    else {
        suffixes = fileSuffixesIt.value();
    }

    const int numSuffixes = suffixes.size();
    if (numSuffixes == 0) {
        QNDEBUG("Can't find any file suffix for mime type " << mimeTypeName
                << ", will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    bool hasNonEmptySuffix = false;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            QNTRACE("Found empty file suffix within suffixes, skipping it");
            continue;
        }

        hasNonEmptySuffix = true;
        break;
    }

    if (!hasNonEmptySuffix) {
        QNDEBUG("All file suffixes for mime type " << mimeTypeName << " are empty, will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    QString fakeFilesStoragePath = applicationPersistentStoragePath();
    fakeFilesStoragePath.append("/fake_files");

    QDir fakeFilesDir(fakeFilesStoragePath);
    if (!fakeFilesDir.exists())
    {
        QNDEBUG("Fake files storage path doesn't exist yet, will attempt to create it");
        if (!fakeFilesDir.mkpath(fakeFilesStoragePath)) {
            QNWARNING("Can't create fake files storage path folder");
            return m_fallbackResourceIcon;
        }
    }

    QString filename("fake_file");
    QFileInfo fileInfo;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            continue;
        }

        fileInfo.setFile(fakeFilesDir, filename + "." + suffix);
        if (fileInfo.exists() && !fileInfo.isFile())
        {
            if (!fakeFilesDir.rmpath(fakeFilesStoragePath + "/" + filename + "." + suffix)) {
                QNWARNING("Can't remove directory " << fileInfo.absolutePath()
                          << " which should not be here in the first place...");
                continue;
            }
        }

        if (!fileInfo.exists())
        {
            QFile fakeFile(fakeFilesStoragePath + "/" + filename + "." + suffix);
            if (!fakeFile.open(QIODevice::ReadWrite)) {
                QNWARNING("Can't open file " << fakeFilesStoragePath << "/" << filename
                          << "." << suffix << " for writing ");
                continue;
            }
        }

        QFileIconProvider fileIconProvider;
        QIcon icon = fileIconProvider.icon(fileInfo);
        if (icon.isNull()) {
            QNTRACE("File icon provider returned null icon for file with suffix " << suffix);
        }

        QNTRACE("Returning the icon from file icon provider for mime type " << mimeTypeName);
        return icon;
    }

    QNTRACE("Couldn't find appropriate icon from either icon theme or fake file with QFileIconProvider, "
            "using \"unknown\" icon as a last resort");
    return m_fallbackResourceIcon;
}

QStringList NoteEditorPluginFactoryPrivate::getFileSuffixesForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::getFileSuffixesForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName);
        return QStringList();
    }

    return mimeType.suffixes();
}

QString NoteEditorPluginFactoryPrivate::getFilterStringForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::getFilterStringForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName);
        return QString();
    }

    return mimeType.filterString();
}

} // namespace qute_note

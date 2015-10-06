#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__I_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__I_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>
#include <QWidget>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)

/**
 * @brief INoteEditorEncryptedAreaPlugin class is the interface for note editor plugin
 * implementing the widget displaying the encrypted areas within the note
 */
class QUTE_NOTE_EXPORT INoteEditorEncryptedAreaPlugin: public QWidget
{
protected:
    INoteEditorEncryptedAreaPlugin(QWidget * parent = Q_NULLPTR);

public:
    /**
     * @brief clone - pure virtual method cloning the current plugin
     * @return pointer to the new clone of the plugin. NOTE: it is
     * caller's responsibility to take care about the ownership
     * of the returned pointer
     */
    virtual INoteEditorEncryptedAreaPlugin * clone() const = 0;

    /**
     * @brief initialize - the method used to initialize the plugin
     * @param parameterNames - names of string parameters stored in HTML <object> tag for the plugin
     * @param parameterValues - values of string parameters stored in HTML <object> tag for the plugin
     * @param pluginFactory - plugin factory object which initializes plugins; here intended to be used
     * for setting up the signal-slot connections, if necessary
     * @param errorDescription - error description in case the plugin can't be initialized properly
     * with this set of parameters
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize(const QStringList & parameterNames, const QStringList & parameterValues,
                            const NoteEditorPluginFactory & pluginFactory, QString & errorDescription) = 0;

    /**
     * @brief name - the method returning the name of the plugin
     * @return the name of the plugin
     */
    virtual QString name() const = 0;

    /**
     * @brief description - the optional method
     * @return plugin's description
     */
    virtual QString description() const { return QString(); }
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__I_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H

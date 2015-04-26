#include "NoteEditor.h"
#include "INoteEditorResourceInserter.h"
#include "NoteEditorPage.h"
#include <client/types/Note.h>
#include <logging/QuteNoteLogger.h>
#include <tools/DesktopServices.h>
#include <QWebFrame>
#include <QByteArray>
#include <QMimeType>
#include <QMimeDatabase>
#include <QDropEvent>
#include <QUrl>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QCryptographicHash>
#include <QImage>
#include <QDir>
#include <QMimeData>

#define CHECKBOX_UNCHECKED_FILE_NAME "checkbox_unchecked.png"
#define CHECKBOX_CHECKED_FILE_NAME "checkbox_checked.png"

namespace qute_note {

NoteEditor::NoteEditor(QWidget * parent) :
    QWebView(parent),
    m_jQuery(),
    m_resizableColumnsPlugin(),
    m_onFixedWidthTableResize(),
    m_pNote(nullptr),
    m_modified(false),
    m_noteEditorResourceInserters(),
    m_lastFreeImageId(0),
    m_lastFreeTableId(0)
{
    NoteEditorPage * page = new NoteEditorPage(*this);
    page->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    page->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    page->setContentEditable(true);
    setPage(page);

    // Setting initial "blank" page, it is of great importance in order to make image insertion work
    setHtml("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
            "<html><head>"
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">"
            "<title></title>"
            "</head>"
            "<body>"
            "</body></html>");

    setAcceptDrops(true);

    __initNoteEditorResources();

    QFile file(":/javascript/jquery/jquery-2.1.3.min.js");
    file.open(QIODevice::ReadOnly);
    m_jQuery = file.readAll();
    file.close();

    file.setFileName(":/javascript/colResizable/colResizable-1.5.min.js");
    file.open(QIODevice::ReadOnly);
    m_resizableColumnsPlugin = file.readAll();
    file.close();

    file.setFileName(":/onFixedWidthTableResize.js");
    file.open(QIODevice::ReadOnly);
    m_onFixedWidthTableResize = file.readAll();
    file.close();

    QObject::connect(this, SIGNAL(loadFinished(bool)), this, SLOT(onNoteLoadFinished(bool)));
    QObject::connect(page, SIGNAL(contentsChanged()), this, SIGNAL(contentChanged()));

    // TODO: temporary thing for debugging/development, remove later
    onNoteLoadFinished(true);
}

NoteEditor::~NoteEditor()
{
}

void NoteEditor::setNote(const Note & note)
{
    if (!m_pNote) {
        m_pNote = new Note(note);
    }
    else {
        *m_pNote = note;
    }

    // TODO: launch note's ENML and resources conversion to widget's content
}

const Note * NoteEditor::getNote()
{
    if (!m_pNote) {
        return nullptr;
    }

    if (m_modified) {
        // TODO: convert widget's content to Note's ENML and resources
    }

    return m_pNote;
}

bool NoteEditor::isModified() const
{
    return m_modified;
}

void NoteEditor::addResourceInserterForMimeType(const QString & mimeTypeName,
                                                    INoteEditorResourceInserter * pResourceInserter)
{
    QNDEBUG("NoteEditor::addResourceInserterForMimeType: mime type = " << mimeTypeName);

    if (!pResourceInserter) {
        QNINFO("Detected attempt to add null pointer to resource inserter for mime type " << mimeTypeName);
        return;
    }

    m_noteEditorResourceInserters[mimeTypeName] = pResourceInserter;
}

bool NoteEditor::removeResourceInserterForMimeType(const QString & mimeTypeName)
{
    QNDEBUG("NoteEditor::removeResourceInserterForMimeType: mime type = " << mimeTypeName);

    auto it = m_noteEditorResourceInserters.find(mimeTypeName);
    if (it == m_noteEditorResourceInserters.end()) {
        QNTRACE("Can't remove resource inserter for mime type " << mimeTypeName << ": not found");
        return false;
    }

    Q_UNUSED(m_noteEditorResourceInserters.erase(it));
    return true;
}

bool NoteEditor::hasResourceInsertedForMimeType(const QString & mimeTypeName) const
{
    auto it = m_noteEditorResourceInserters.find(mimeTypeName);
    return (it != m_noteEditorResourceInserters.end());
}

void NoteEditor::undo()
{
    page()->triggerAction(QWebPage::Undo);
}

void NoteEditor::redo()
{
    page()->triggerAction(QWebPage::Redo);
}

void NoteEditor::cut()
{
    page()->triggerAction(QWebPage::Cut);
}

void NoteEditor::copy()
{
    page()->triggerAction(QWebPage::Copy);
}

void NoteEditor::paste()
{
    page()->triggerAction(QWebPage::Paste);
}

void NoteEditor::textBold()
{
    page()->triggerAction(QWebPage::ToggleBold);
}

void NoteEditor::textItalic()
{
    page()->triggerAction(QWebPage::ToggleItalic);
}

void NoteEditor::textUnderline()
{
    page()->triggerAction(QWebPage::ToggleUnderline);
}

void NoteEditor::textStrikethrough()
{
    page()->triggerAction(QWebPage::ToggleStrikethrough);
}

void NoteEditor::alignLeft()
{
    page()->triggerAction(QWebPage::AlignLeft);
}

void NoteEditor::alignCenter()
{
    page()->triggerAction(QWebPage::AlignCenter);
}

void NoteEditor::alignRight()
{
    page()->triggerAction(QWebPage::AlignRight);
}

void NoteEditor::insertToDoCheckbox()
{
    QString imageId = QString::number(m_lastFreeImageId++);

    QString html = "<img id=\"";
    html += imageId;
    html += "\" src=\"qrc:/checkbox_icons/checkbox_no.png\" style=\"margin:0px 4px\" "
            "onmouseover=\"JavaScript:this.style.cursor=\\'default\\'\" "
            "onclick=\"JavaScript:if(document.getElementById(\\'";
    html += imageId;
    html += "\\').src ==\\'qrc:/checkbox_icons/checkbox_no.png\\') "
            "document.getElementById(\\'";
    html += imageId;
    html += "\\').src=\\'qrc:/checkbox_icons/checkbox_yes.png\\'; "
            "else document.getElementById(\\'";
    html += imageId;
    html += "\\').src=\\'qrc:/checkbox_icons/checkbox_no.png\\';\" />";

    execJavascriptCommand("insertHtml", html);
}

void NoteEditor::setSpellcheck(const bool enabled)
{
    // TODO: implement
    Q_UNUSED(enabled)
}

void NoteEditor::setFont(const QFont & font)
{
    QString fontName = font.family();
    execJavascriptCommand("fontName", fontName);
}

void NoteEditor::setFontHeight(const int height)
{
    if (height > 0) {
        execJavascriptCommand("fontSize", QString::number(height));
    }
    else {
        QString error = QT_TR_NOOP("Detected incorrect font size: " + QString::number(height));
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditor::setFontColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("foreColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid font color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditor::setBackgroundColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("hiliteColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid background color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditor::insertHorizontalLine()
{
    execJavascriptCommand("insertHorizontalRule");
}

void NoteEditor::changeIndentation(const bool increase)
{
    execJavascriptCommand((increase ? "indent" : "outdent"));
}

void NoteEditor::insertBulletedList()
{
    execJavascriptCommand("insertUnorderedList");
}

void NoteEditor::insertNumberedList()
{
    execJavascriptCommand("insertOrderedList");
}

#define CHECK_NUM_COLUMNS() \
    if (columns <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of columns: ") + QString::number(columns); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

#define CHECK_NUM_ROWS() \
    if (rows <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of rows: ") + QString::number(rows); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void NoteEditor::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    int pageWidth = geometry().width();
    if (widthInPixels > 2 * pageWidth) {
        QString error = QT_TR_NOOP("Can't insert table, width is too large (more than twice the page width): ") + QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (widthInPixels <= 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width: ") + QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int singleColumnWidth = widthInPixels / columns;
    if (singleColumnWidth == 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width for specified number of columns (single column width is zero): width = ") +
                        QString::number(widthInPixels) + QT_TR_NOOP(", number of columns = ") + QString::number(columns);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    size_t tableId = m_lastFreeTableId++;
    QString htmlTable = composeHtmlTable(widthInPixels, singleColumnWidth, rows, columns,
                                         /* relative = */ false, tableId);
    execJavascriptCommand("insertHTML", htmlTable);

    QString colResizable = "$(\"#";
    colResizable += QString::number(tableId);
    colResizable += "\").colResizable({";
    colResizable += "liveDrag:true, ";
    colResizable += "gripInnerHtml:\"<div class=\\'grip\\'></div>\", ";
    colResizable += "draggingClass:\"dragging\", ";
    colResizable += "postbackSafe:true, ";
    colResizable += "onResize:onFixedWidthTableResized ";
    colResizable += "});";
    QNTRACE("colResizable js code: " << colResizable);
    page()->mainFrame()->evaluateJavaScript(colResizable);
}

void NoteEditor::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    if (relativeWidth <= 0.01) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too small: ") + QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (relativeWidth > 100.0 +1.0e-9) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too large: ") + QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    double singleColumnWidth = relativeWidth / columns;

    size_t tableId = m_lastFreeTableId++;
    QString htmlTable = composeHtmlTable(relativeWidth, singleColumnWidth, rows, columns,
                                         /* relative = */ true, tableId);
    execJavascriptCommand("insertHTML", htmlTable);

    QString colResizable = "$(\"#";
    colResizable += QString::number(tableId);
    colResizable += "\").colResizable({";
    colResizable += "liveDrag:true, ";
    colResizable += "gripInnerHtml:\"<div class=\\'grip\\'></div>\", ";
    colResizable += "draggingClass:\"dragging\", ";
    colResizable += "postbackSafe:true, ";
    colResizable += "fixed:false";
    colResizable += "});";
    QNTRACE("colResizable js code: " << colResizable);
    page()->mainFrame()->evaluateJavaScript(colResizable);
}

void NoteEditor::onNoteLoadCancelled()
{
    // TODO: implement
}

void NoteEditor::onNoteLoadFinished(bool ok)
{
    if (!ok) {
        QNWARNING("Note page was not loaded successfully");
        return;
    }

    QWebFrame * frame = page()->mainFrame();
    if (!frame) {
        return;
    }

    frame->evaluateJavaScript(m_jQuery);
    frame->evaluateJavaScript(m_resizableColumnsPlugin);
    frame->evaluateJavaScript(m_onFixedWidthTableResize);
    QNTRACE("Evaluated jQuery and colResizable plugin");
}

void NoteEditor::execJavascriptCommand(const QString & command)
{
    QWebFrame * frame = page()->mainFrame();
    QString javascript = QString("document.execCommand(\"%1\", false, null)").arg(command);
    frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript);
}

void NoteEditor::execJavascriptCommand(const QString & command, const QString & args)
{
    QWebFrame * frame = page()->mainFrame();
    QString javascript = QString("document.execCommand('%1', false, '%2')").arg(command).arg(args);
    frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript);
}

void NoteEditor::dropEvent(QDropEvent * pEvent)
{
    QNDEBUG("NoteEditor::dropEvent");

    if (!pEvent) {
        QNINFO("Null drop event was detected");
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (!pMimeData) {
        QNINFO("Null mime data from drop event was detected");
        return;
    }

    QList<QUrl> urls = pMimeData->urls();
    typedef QList<QUrl>::iterator Iter;
    Iter urlsEnd = urls.end();
    for(Iter it = urls.begin(); it != urlsEnd; ++it)
    {
        QString url = it->toString();
        if (url.toLower().startsWith("file://")) {
            url.remove(0,6);
            dropFile(url);
        }
    }
}

void NoteEditor::dropFile(QString & filepath)
{
    QNDEBUG("NoteEditor::dropFile: " << filepath);

    QFileInfo fileInfo(filepath);
    if (!fileInfo.isFile()) {
        QNINFO("Detected attempt to drop something else rather than file: " << filepath);
        return;
    }

    if (!fileInfo.isReadable()) {
        QNINFO("Detected attempt to drop file which is not readable: " << filepath);
        return;
    }

    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);

    if (!mimeType.isValid()) {
        QNINFO("Detected invalid mime type for file " << filepath);
        return;
    }

    QByteArray data = QFile(filepath).readAll();
    QString dataHash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    attachResourceToNote(data, dataHash, mimeType);

    QString mimeTypeName = mimeType.name();
    auto resourceInsertersEnd = m_noteEditorResourceInserters.end();
    for(auto it = m_noteEditorResourceInserters.begin(); it != resourceInsertersEnd; ++it)
    {
        if (mimeTypeName != it.key()) {
            continue;
        }

        QNTRACE("Found resource inserter for mime type " << mimeTypeName);
        INoteEditorResourceInserter * pResourceInserter = it.value();
        if (!pResourceInserter) {
            QNINFO("Detected null pointer to registered resource inserter for mime type " << mimeTypeName);
            continue;
        }

        pResourceInserter->insertResource(data, mimeType);
        return;
    }

    // If we are still here, no specific resource inserter was found, so will process two generic cases: image or any other type
    if (mimeTypeName.startsWith("image")) {
        insertImage(data, dataHash, mimeType);
        return;
    }

    // TODO: process generic mime type
}

void NoteEditor::attachResourceToNote(const QByteArray & data, const QString & dataHash, const QMimeType & mimeType)
{
    // TODO: implement
}

template <typename T>
QString NoteEditor::composeHtmlTable(const T width, const T singleColumnWidth,
                                     const int rows, const int columns,
                                     const bool relative, const size_t tableId)
{
    // Table header
    QString htmlTable = "<table style=\"border-collapse: collapse; margin-left: 0px; table-layout: fixed; width: ";
    htmlTable += QString::number(width);
    if (relative) {
        htmlTable += "%";
    }
    else {
        htmlTable += "px";
    }
    htmlTable += ";\" ";

    htmlTable += "id=\"";
    htmlTable += QString::number(tableId);
    htmlTable += "\"><tbody>";

    for(int i = 0; i < rows; ++i)
    {
        // Row header
        htmlTable += "<tr>";

        for(int j = 0; j < columns; ++j)
        {
            // Column header
            htmlTable += "<td style=\"border: 1px solid rgb(219, 219, 219); padding: 10 px; margin: 0px; width: ";
            htmlTable += QString::number(singleColumnWidth);
            if (relative) {
                htmlTable += "%";
            }
            else {
                htmlTable += "px";
            }
            htmlTable += ";\">";

            // Blank line to preserve the size
            htmlTable += "<div><br></div>";

            // End column
            htmlTable += "</td>";
        }

        // End row
        htmlTable += "</tr>";
    }

    // End table
    htmlTable += "</tbody></table>";
    return htmlTable;
}

void NoteEditor::insertImage(const QByteArray & data, const QString & dataHash, const QMimeType & mimeType)
{
    QNDEBUG("NoteEditor::insertImage: data hash = " << dataHash << ", mime type = " << mimeType.name());

    // Write the data to the temporary file
    // TODO: first check if there is the existing temporary file with this resource's data
    QTemporaryFile tmpFile;
    if (!tmpFile.open()) {
        QString error = QT_TR_NOOP("Can't render dropped image: can't create temporary file: ");
        error += tmpFile.errorString();
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    tmpFile.write(data);
    // TODO: put the path to the file to the local cache by resource's hash

    QString imageHtml = "<img src=\"file://";
    imageHtml += QFileInfo(tmpFile).absolutePath();
    imageHtml += "\" type=\"";
    imageHtml += mimeType.name();
    imageHtml += "\" hash=\"";
    // TODO: figure out how to deal with the context menu
    // TODO: idea: on hover can change cursor to "zoom" one and on LMB click can zoom the area of the image
    imageHtml += "\" />";

    execJavascriptCommand("insertHTML", imageHtml);
}

} // namespace qute_note

void __initNoteEditorResources()
{
    Q_INIT_RESOURCE(checkbox_icons);
    Q_INIT_RESOURCE(jquery);
    Q_INIT_RESOURCE(colResizable);
}

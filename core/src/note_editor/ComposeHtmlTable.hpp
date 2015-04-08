#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__COMPOSE_HTML_TABLE_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__COMPOSE_HTML_TABLE_H

#include <QString>

namespace note_editor_controller_private {

template <typename T>
QString composeHtmlTable(const T width, const T singleColumnWidth, const int rows, const int columns,
                         const bool relative)
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
    htmlTable += ";\"><tbody>";

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

}

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__COMPOSE_HTML_TABLE_H

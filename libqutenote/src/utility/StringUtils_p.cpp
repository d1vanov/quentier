#include "StringUtils_p.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QRegExp>

namespace qute_note {

StringUtilsPrivate::StringUtilsPrivate()
{}

void StringUtilsPrivate::removePunctuation(QString & str, const QVector<QChar> & charactersToPreserve) const
{
    QString filterStr = QString::fromUtf8("[`~!@#$%^&()—+=|:;<>«»,.?/{}\'\"\\[\\]]");

    for(auto it = charactersToPreserve.begin(), end = charactersToPreserve.end(); it != end; ++it)
    {
        int pos = -1;
        while((pos = filterStr.indexOf(*it)) >= 0) {
            filterStr.remove(pos, 1);
        }
    }

    QRegExp punctuationFilter(filterStr);
    str.remove(punctuationFilter);
}

} // namespace qute_note

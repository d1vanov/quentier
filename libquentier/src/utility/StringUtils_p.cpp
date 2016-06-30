#include "StringUtils_p.h"
#include <quentier/logging/QuentierLogger.h>
#include <QRegExp>

namespace quentier {

StringUtilsPrivate::StringUtilsPrivate() :
    m_diacriticLetters(),
    m_noDiacriticLetters()
{
    initialize();
}

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

void StringUtilsPrivate::removeDiacritics(QString & str) const
{
    QNTRACE("str before normalizing by D form: " << str);
    str = str.normalized(QString::NormalizationForm_KD);
    QNTRACE("str after normalizing by KD form: " << str);

    for(int i = 0; i < str.length(); ++i)
    {
        QChar currentCharacter = str[i];
        QChar::Category category = currentCharacter.category();
        if ( (category == QChar::Mark_NonSpacing) ||
             (category == QChar::Mark_SpacingCombining) ||
             (category == QChar::Mark_Enclosing) )
        {
            str.remove(i, 1);
            continue;
        }

        int diacriticIndex = m_diacriticLetters.indexOf(currentCharacter);
        if (diacriticIndex < 0) {
            continue;
        }

        const QString & replacement = m_noDiacriticLetters[diacriticIndex];
        str.replace(i, 1, replacement);
    }

    QNTRACE("str after removing diacritics: " << str);
}

void StringUtilsPrivate::initialize()
{
    m_diacriticLetters = QString::fromUtf8("ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ");
    m_noDiacriticLetters.reserve(m_diacriticLetters.size());
    m_noDiacriticLetters << "S" << "OE" << "Z" << "s" << "oe" << "z" << "Y" << "Y" << "u" << "A" << "A" << "A" << "A" << "A"
                         << "A" << "AE" << "C" << "E" << "E" << "E" << "E" << "I" << "I" << "I" << "I" << "D" << "N" << "O"
                         << "O" << "O" << "O" << "O" << "O" << "U" << "U" << "U" << "U" << "Y" << "s" << "a" << "a" << "a"
                         << "a" << "a" << "a" << "ae" << "c" << "e" << "e" << "e" << "e" << "i" << "i" << "i" << "i" << "o"
                         << "n" << "o" << "o" << "o" << "o" << "o" << "o" << "u" << "u" << "u" << "u" << "y" << "y";
}

} // namespace quentier

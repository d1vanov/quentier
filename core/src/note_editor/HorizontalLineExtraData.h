#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__HORIZONTAL_LINE_EXTRA_DATA_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__HORIZONTAL_LINE_EXTRA_DATA_H

#include <tools/Linkage.h>
#include <QTextBlockUserData>

class QUTE_NOTE_EXPORT HorizontalLineExtraData : public QTextBlockUserData
{
public:
    HorizontalLineExtraData();
    virtual ~HorizontalLineExtraData() override;
};

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__HORIZONTAL_LINE_EXTRA_DATA_H

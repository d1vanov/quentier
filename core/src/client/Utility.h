#ifndef __QUTE_NOTE__CLIENT__UTILITY_H
#define __QUTE_NOTE__CLIENT__UTILITY_H

#include <Types_types.h>

namespace qute_note {

bool CheckGuid(const evernote::edam::Guid & guid);

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber);

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__UTILITY_H

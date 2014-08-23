/*
 * I can't be silent about what M$ compiler forces me to do: first, it doesn't tolerate
 * header-only classes without explicitly defined constructors in a dynamic library. Second,
 * it doesn't tolerate header-only classes with explicilty defaulted constructors in a dynamic library.
 * Therefore I have to write these dummy constructor and destructor to make M$ compiler happy with them.
 */

#include "HorizontalLineExtraData.h"

HorizontalLineExtraData::HorizontalLineExtraData()
{}

HorizontalLineExtraData::~HorizontalLineExtraData()
{}

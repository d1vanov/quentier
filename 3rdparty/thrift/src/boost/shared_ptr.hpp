#ifndef __QUTE_NOTE__THIRDPARTY__BOOST_TO_STD_SHARED_PTR_H
#define __QUTE_NOTE__THIRDPARTY__BOOST_TO_STD_SHARED_PTR_H

#include <memory>

namespace boost {

template <class T>
using shared_ptr = std::shared_ptr<T>;

}

#endif // __QUTE_NOTE__THIRDPARTY__BOOST_TO_STD_SHARED_PTR_H

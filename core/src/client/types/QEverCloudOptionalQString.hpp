#ifndef __QUTE_NOTE__CLIENT__TYPES__QEVERCLOUD_OPTIONAL_QSTRING_H
#define __QUTE_NOTE__CLIENT__TYPES__QEVERCLOUD_OPTIONAL_QSTRING_H

template<> inline qevercloud::Optional<QString> & qevercloud::Optional<QString>::operator=(const QString & value) {
    value_ = value;
    isSet_ = !value.isEmpty();
    return *this;
}

#endif // __QUTE_NOTE__CLIENT__TYPES__QEVERCLOUD_OPTIONAL_QSTRING_H

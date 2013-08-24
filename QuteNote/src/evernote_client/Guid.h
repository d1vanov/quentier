#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__GUID_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__GUID_H

#include <string>
#include <memory>

namespace qute_note {

class Guid
{
public:
    Guid();

    /**
     * @brief Guid - create Guid object from string
     * @param guid - string to be represented as guid
     */
    explicit Guid(const std::string & guid);

    /**
     * @brief isEmpty - checks whether Guid is empty
     * @return true if Guid is empty, false otherwise
     */
    bool isEmpty() const;

    /**
     * @brief operator == - checks whether Guid is equal to another Guid
     * @param other - the other Guid for comparison with current one
     * @return true if two Guids are identical, false otherwise
     */
    bool operator ==(const Guid & other) const;

    /**
     * @brief operator != - checks whether Guid is not equal to another Guid
     * @param other - the other Guid for comparison with the current one
     * @return true if two Guids differ, false otherwise
     */
    bool operator !=(const Guid & other) const;

    /**
     * @brief operator < - checks whether current Guid is less than another Guid,
     * used for sorting of Evernote data elements by Guid
     * @param other - the other Guid for comparison with the current one
     * @return true if current Guid is less than the other one, false otherwise
     */
    bool operator <(const Guid & other) const;

private:
    Guid(const Guid & other) = delete;
    Guid & operator=(const Guid & other) = delete;

    std::string m_guid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__GUID_H

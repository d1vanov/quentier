#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__LOCATION_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__LOCATION_H

namespace qute_note {

class Location
{
public:
    Location();
    Location(const double latitude, const double longitude, const double altitude);
    Location(const Location & location);
    Location & operator=(const Location & location);

    double latitude() const;
    void setLatitude(const double latitude);

    double longitude() const;
    void setLongitude(const double longitude);

    double altitude() const;
    void setAltitude(const double altitude);

    bool isValid() const;

private:
    double m_latitude;
    double m_longitude;
    double m_altitude;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__LOCATION_H

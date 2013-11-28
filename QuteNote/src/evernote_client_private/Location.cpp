#include "Location.h"
#include <cmath>

namespace qute_note {

Location::Location() :
    m_latitude(0.0),
    m_longitude(0.0),
    m_altitude(0.0)
{}

Location::Location(const double latitude, const double longitude, const double altitude) :
    m_latitude(latitude),
    m_longitude(longitude),
    m_altitude(altitude)
{}

Location::Location(const Location & location) :
    m_latitude(location.m_latitude),
    m_longitude(location.m_longitude),
    m_altitude(location.m_altitude)
{}

Location & Location::operator=(const Location & location)
{
    if (this != &location)
    {
        m_latitude = location.m_latitude;
        m_longitude = location.m_longitude;
        m_altitude = location.m_altitude;
    }

    return *this;
}

double Location::latitude() const
{
    return m_latitude;
}

void Location::setLatitude(const double latitude)
{
    m_latitude = latitude;
}

double Location::longitude() const
{
    return m_longitude;
}

void Location::setLongitude(const double longitude)
{
    m_longitude = longitude;
}

double Location::altitude() const
{
    return m_altitude;
}

void Location::setAltitude(const double altitude)
{
    m_altitude = altitude;
}

bool Location::isValid() const
{
    const double zero_tol = 1e-14;

    if ( (std::fabs(m_latitude) < zero_tol) &&
         (std::fabs(m_longitude) < zero_tol) &&
         (std::fabs(m_altitude) < zero_tol) )
    {
        return false;
    }
    else
    {
        return true;
    }
}

}

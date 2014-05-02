#ifndef QEVERCLOUD_GLOBALS_H
#define QEVERCLOUD_GLOBALS_H

#include <QNetworkAccessManager>

/**
 * All the library lives in this namespace.
 */
namespace qevercloud {

/**
 * All network request made by QEverCloud - including OAuth - are
 * served by this NetworkAccessManager.
 *
 * Use this function to handle proxy authentication requests etc.
 */
QNetworkAccessManager* evernoteNetworkAccessManager();

/**
 * qevercloud library version.
 */
extern const int libraryVersion;



}

#endif // QEVERCLOUD_GLOBALS_H

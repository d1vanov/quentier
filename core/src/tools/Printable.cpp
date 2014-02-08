#include "Printable.h"
#include <Types_types.h>

namespace qute_note {

Printable::~Printable()
{}

const QString Printable::ToQString() const
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << *this;
    return str;
}

QTextStream & operator <<(QTextStream & strm,
                          const Printable & printable)
{
    return printable.Print(strm);
}

} // namespace qute_note

#define CHECK_AND_PRINT_ATTRIBUTE(object, name, ...) \
    bool isSet##name = isSet.name; \
    strm << #name " is " << (isSet##name ? "set" : "not set") << "\n"; \
    if (isSet##name) { \
        strm << #name " = " << __VA_ARGS__(object.name) << "\n"; \
    }

QTextStream & operator <<(QTextStream & strm, const evernote::edam::BusinessUserInfo & info)
{
    strm << "BusinessUserInfo: {\n";

    const auto & isSet = info.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(info, businessId);
    CHECK_AND_PRINT_ATTRIBUTE(info, businessName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(info, role, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(info, email, QString::fromStdString);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::PremiumInfo & info)
{
    strm << "PremiumUserInfo {\n";

    const auto & isSet = info.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(info, premiumExpirationDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(info, sponsoredGroupName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(info, sponsoredGroupRole, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(info, premiumUpgradable);

    strm << "premiumExtendable = " << info.premiumExtendable << "\n";
    strm << "premiumPending = " << info.premiumPending << "\n";
    strm << "premiumCancellationPending = " << info.premiumCancellationPending << "\n";
    strm << "canPurchaseUploadAllowance = " << info.canPurchaseUploadAllowance << "\n";

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::Accounting & accounting)
{
    strm << "Accounting: {\n";

    const auto & isSet = accounting.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(accounting, uploadLimit, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, uploadLimitEnd, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, uploadLimitNextMonth, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumServiceStatus, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumOrderNumber, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumCommerceService, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumServiceStart, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumServiceSKU, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastSuccessfulCharge, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastFailedCharge, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastFailedChargeReason, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, nextPaymentDue, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumLockUntil, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, updated, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, premiumSubscriptionNumber, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, lastRequestedCharge, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, currency, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, unitPrice, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, businessId, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, businessName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, businessRole, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, unitDiscount, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(accounting, nextChargeDate, static_cast<qint64>);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::UserAttributes & attributes)
{
    strm << "UserAttributes: {\n";

    const auto & isSet = attributes.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(attributes, defaultLocationName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, defaultLatitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, defaultLongitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, preactivation);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, incomingEmailAddress, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, comments, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, dateAgreedToTermsOfService, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, maxReferrals);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, referralCount);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, refererCode, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sentEmailDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sentEmailCount);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, dailyEmailLimit);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, emailOptOutDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, partnerEmailOptInDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, preferredLanguage, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, preferredCountry, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, clipFullPage);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, twitterUserName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, twitterId, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, groupName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, recognitionLanguage, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, referralProof, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, educationalDiscount);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, businessAddress, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, hideSponsorBilling);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, taxExempt);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, useEmailAutoFiling);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderEmailConfig, static_cast<quint8>);

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::NoteAttributes & attributes)
{
    strm << "NoteAttributes: {\n";

    const auto & isSet = attributes.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(attributes, subjectDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, latitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, longitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, altitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, author, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, source, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceURL, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceApplication, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, shareDate, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderOrder, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderDoneTime, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, reminderTime, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, placeName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, contentClass, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, lastEditedBy, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, creatorId, static_cast<qint32>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, lastEditorId, static_cast<qint32>);

    strm << "applicationData is " << (isSet.applicationData ? "set" : "not set") << "\n";
    if (isSet.applicationData)
    {
        const auto & applicationData = attributes.applicationData;
        const auto & keysOnly = applicationData.keysOnly;
        const auto & fullMap = applicationData.fullMap;

        strm << "ApplicationData: keys only: \n";
        for(const auto & key: keysOnly) {
            strm << QString::fromStdString(key) << "; ";
        }
        strm << "\n";

        strm << "ApplicationData: full map: \n";
        for(const auto & pair: fullMap) {
            strm << "(" << QString::fromStdString(pair.first)
                 << ", " << QString::fromStdString(pair.second) << "); ";
        }
        strm << "\n";
    }

    strm << "classificatiopns are " << (isSet.classifications ? "set" : "not set") << "\n";
    if (isSet.classifications)
    {
        const auto & classifications = attributes.classifications;
        strm << "Classifications: \n";
        for(const auto & pair: classifications) {
            strm << "(" << QString::fromStdString(pair.first)
                 << ", " << QString::fromStdString(pair.second) << "); ";
        }
        strm << "\n";
    }

    strm << "}; \n";
    return strm;
}

QTextStream & operator <<(QTextStream & strm, const evernote::edam::ResourceAttributes & attributes)
{
    strm << "ResourceAttributes: {\n";

    const auto & isSet = attributes.__isset;

    CHECK_AND_PRINT_ATTRIBUTE(attributes, sourceURL, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, timestamp, static_cast<qint64>);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, latitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, longitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, altitude);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, cameraMake, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, cameraModel, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, clientWillIndex);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, recoType, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, fileName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(attributes, attachment);

    strm << "applicationData is " << (isSet.applicationData ? "set" : "not set") << "\n";
    if (isSet.applicationData)
    {
        const auto & applicationData = attributes.applicationData;
        const auto & keysOnly = applicationData.keysOnly;
        const auto & fullMap = applicationData.fullMap;

        strm << "ApplicationData: keys only: \n";
        for(const auto & key: keysOnly) {
            strm << QString::fromStdString(key) << "; ";
        }
        strm << "\n";

        strm << "ApplicationData: full map: \n";
        for(const auto & pair: fullMap) {
            strm << "(" << QString::fromStdString(pair.first)
                 << ", " << QString::fromStdString(pair.second) << "); ";
        }
        strm << "\n";
    }

    strm << "}; \n";
    return strm;
}

#undef CHECK_AND_PRINT_ATTRIBUTE

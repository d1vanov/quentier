cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_account)

set(HEADERS
    AccountManager.h
    AddAccountDialog.h
    DeleteAccountDialog.h
    ManageAccountsDialog.h
    AccountModel.h
    AccountFilterModel.h
    AccountDelegate.h)

set(SOURCES
    AccountManager.cpp
    AddAccountDialog.cpp
    DeleteAccountDialog.cpp
    ManageAccountsDialog.cpp
    AccountModel.cpp
    AccountFilterModel.cpp
    AccountDelegate.cpp)

set(FORMS
    AddAccountDialog.ui
    DeleteAccountDialog.ui
    ManageAccountsDialog.ui)

qt_wrap_ui(FORMS_HEADERS ${FORMS})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

set(quentier_account ${PROJECT_NAME} CACHE INTERNAL "quentier_account")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})

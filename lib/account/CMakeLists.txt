cmake_minimum_required(VERSION 3.16.3)

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

if(BUILD_WITH_QT6)
  qt6_wrap_ui(FORMS_HEADERS ${FORMS})
else()
  qt5_wrap_ui(FORMS_HEADERS ${FORMS})
endif()

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
	PUBLIC
	  Qt6::Core
	  Qt6::Widgets)
else()
  target_link_libraries(${PROJECT_NAME}
	PUBLIC
	  Qt5::Core
	  Qt5::Widgets)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_account ${PROJECT_NAME} CACHE INTERNAL "quentier_account")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)

#-------------------------------------------------
#
# Project created by QtCreator 2022-06-20T13:12:12
#
#-------------------------------------------------

QT       += core gui sql network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Screenshot_Manager
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        hashtag_button_db.cpp \
        hashtag_dialog.cpp \
        hashtag_poll_button.cpp \
        hashtag_poll_dialog.cpp \
        hashtag_preview_db.cpp \
        locations.cpp \
        poll_preparation_db.cpp \
        record_item_db.cpp \
        record_poll_preview.cpp \
        record_preview_db.cpp \
        record_preview_info.cpp \
        records_dialog.cpp \
        release_preparation_db.cpp \
        series_dialog.cpp \
        src\recordpreview.cpp \
        src\database.cpp \
        src\abstract_mode.cpp \
        src\common.cpp \
        src\hashtag_button.cpp \
        src\journal_creation.cpp \
        src\journal_reading.cpp \
        main.cpp \
        src\mainwindow.cpp \
        src\record.cpp \
        src\release_preparation.cpp \
        src\text_reading.cpp \
        src\vk_manager.cpp \
        text_labeling.cpp \
        title_group.cpp

HEADERS += \
        hashtag_button_db.h \
        hashtag_dialog.h \
        hashtag_poll_button.h \
        hashtag_poll_dialog.h \
        hashtag_preview_db.h \
        include/hashtag_info.h \
        include/series_info.h \
        include\database.h \
        include\abstract_mode.h \
        include\common.h \
        include\hashtag_button.h \
        include\journal_creation.h \
        include\journal_reading.h \
        include\mainwindow.h \
        include\record.h \
        include\release_preparation.h \
        include\text_reading.h \
        include\vk_manager.h \
        include\recordpreview.h \
        include\query_filters.h \
        locations.h \
        poll_preparation_db.h \
        record_item_db.h \
        record_poll_preview.h \
        record_preview_db.h \
        record_preview_info.h \
        records_dialog.h \
        release_preparation_db.h \
        series_dialog.h \
        text_labeling.h \
        title_group.h

FORMS += \
        hashtag_dialog.ui \
        hashtag_preview_db.ui \
        mainwindow.ui \
        poll_preparation_db.ui \
        record_poll_preview.ui \
        record_preview_db.ui \
        recordpreview.ui \
        records_dialog.ui \
        release_preparation_db.ui \
        series_dialog.ui \
        text_labeling.ui \
        title_group.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc

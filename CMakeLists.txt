cmake_minimum_required(VERSION 3.16)
project(FinTrack VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Network Sql)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/authdialog.cpp
    src/datamanager.cpp
    src/addtransactiondialog.cpp
)

set(HEADERS
    src/mainwindow.h
    src/authdialog.h
    src/datamanager.h
    src/addtransactiondialog.h
)

add_executable(FinTrack ${SOURCES} ${HEADERS})

target_link_libraries(FinTrack PRIVATE
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
    Qt6::Sql
)

target_include_directories(FinTrack PRIVATE src)

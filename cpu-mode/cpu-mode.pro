QT += quick
CONFIG += sailfishapp c++11
TARGET = cpu-mode
SOURCES += src/main.cpp src/backend.cpp
HEADERS += src/backend.h
RESOURCES += qml.qrc

target.path = /usr/bin

qml.path = /usr/share/cpu-mode/qml
qml.files = qml/main.qml qml/FirstPage.qml qml/cover/CoverPage.qml

desktop.path = /usr/share/applications
desktop.files = data/cpu-mode.desktop

icon.path = /usr/share/icons/hicolor/86x86/apps
icon.files = icons/cpu-mode.png

INSTALLS += target qml desktop icon

TEMPLATE = app
TARGET = contactlist
QT += quick

HEADERS += \
    addressmodel.h

SOURCES += \
    main.cpp \
    addressmodel.cpp

RESOURCES += \
   $$files(*.qml)

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD/designer

OTHER_FILES += \
    designer/Backend/*.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols2/contactlist
INSTALLS += target

QT += quick \
      core  \
      network

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += server.h \
    book.h \
    clienthandler.h \
    clothing.h \
    consumer.h \
    filemanager.h \
    food.h \
    merchant.h \
    order.h \
    product.h \
    server.h \
    serverauthmanager.h \
    serverordermanager.h \
    serverproductmanager.h \
    servershoppingcartmanager.h \
    user.h

SOURCES += \
        book.cpp \
        clienthandler.cpp \
        clothing.cpp \
        consumer.cpp \
        filemanager.cpp \
        food.cpp \
        main.cpp \
        merchant.cpp \
        order.cpp \
        product.cpp \
        server.cpp \
        serverauthmanager.cpp \
        serverordermanager.cpp \
        serverproductmanager.cpp \
        servershoppingcartmanager.cpp \
        user.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

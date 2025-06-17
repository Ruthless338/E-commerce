QT += quick \
      quickcontrols2 \
      core \
      network

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
        authmanager.h \
        book.h \
        clothing.h \
        consumer.h \
        filemanager.h \
        food.h \
        globalstate.h \
        main.h \
        merchant.h \
        order.h \
        ordermanager.h \
        product.h \
        productmodel.h \
        shoppingcart.h \
        user.h \
        networkclient.h


SOURCES += \
        authmanager.cpp \
        book.cpp \
        clothing.cpp \
        consumer.cpp \
        filemanager.cpp \
        food.cpp \
        globalstate.cpp \
        main.cpp \
        merchant.cpp \
        order.cpp \
        ordermanager.cpp \
        product.cpp \
        productmodel.cpp \
        shoppingcart.cpp \
        user.cpp \
        networkclient.cpp

RESOURCES += qml.qrc \
             images.qrc \
             data.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

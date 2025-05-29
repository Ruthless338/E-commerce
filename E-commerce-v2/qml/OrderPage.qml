// OrderPage.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ECommerce.Core 1.0

Item {
    id: orderPage
    visible: false
    anchors.fill: parent

    property var orderData: null

    signal goBack()
    signal orderConfirmed()

    Label {
        id: operationStatus
        visible: false
        color: "red"
        anchors.centerIn: parent
        z: 2
        font.pixelSize: 16
        property bool success: false
        
        Timer {
            id: statusTimer
            interval: 2000
            onTriggered: {
                operationStatus.visible = false;
                if (operationStatus.success) {
                    orderPage.visible = false;
                    mainContentPage.visible = true;
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "返回购物车"
                onClicked: goBack()
            }
            Label {
                text: "确认订单"
                font.pixelSize: 20
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
            }
        }

        ListView {
            id: orderListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: orderData ? orderData.items : []
            delegate: Rectangle {
                width: ListView.view.width
                height: 100
                color: index % 2 ? "#f0f0f0" : "white"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15

                    Image {
                        source: modelData.imagePath ? "file:///" + modelData.imagePath : "qrc:/images/placeholder.png"
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 80
                        fillMode: Image.PreserveAspectFit
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        Label {
                            text: modelData.name
                            font.bold: true
                            font.pixelSize: 16
                        }
                        Label {
                            text: modelData.description
                            font.pixelSize: 12
                            color: "#666"
                        }
                        Label {
                            text: "单价：￥" + modelData.price.toFixed(2)
                            font.pixelSize: 14
                            color: "green"
                        }
                        Label {
                            text: "数量：" + modelData.quantity
                            font.pixelSize: 14
                        }
                        Label {
                            text: "小计：￥" + (modelData.price * modelData.quantity).toFixed(2)
                            font.pixelSize: 14
                            font.bold: true
                            color: "#d32f2f"
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#e0e0e0"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 20

                Label {
                    text: "总计：￥" + (orderData ? orderData.totalPrice.toFixed(2) : "0.00")
                    font.bold: true
                    font.pixelSize: 16
                    Layout.alignment: Qt.AlignVCenter
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "确认下单"
                    enabled: orderData !== null
                    onClicked: {
                        if (orderData && orderData.items) {
                            const orderResult = OrderManager.createOrder(orderData.username, orderData.items);
                            if (orderResult) {
                                // 清空购物车
                                ShoppingCart.loadShoppingCart(orderData.username);
                                operationStatus.text = "订单创建成功！";
                                operationStatus.color = "green";
                                operationStatus.success = true;
                            } else {
                                operationStatus.text = "订单创建失败，请检查余额或库存！";
                                operationStatus.color = "red";
                                operationStatus.success = false;
                            }
                            operationStatus.visible = true;
                            statusTimer.start();
                        }
                    }
                }
            }
        }
    }
}

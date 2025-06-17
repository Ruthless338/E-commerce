// OrderPage.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ECommerce.Core 1.0

Item {
    id: orderPage
    visible: false
    anchors.fill: parent

    property var orderObject: null

    signal goBack()
    // signal orderConfirmed()
    signal orderPaymentSuccess()
    signal orderPaymentFailed()

    Connections {
        target: orderPage
        function onOrderObjectChanged() {
            if (orderObject) {
                orderListView.model = orderObject.getQmlItems();
                totalAmountLabel.text = "总计：￥" + orderObject.calculateTotal().toFixed(2);
                countdownLabel.text = "剩余支付时间: " + formatRemainingTime(orderObject.getRemainingSeconds());
                paymentCountdownTimer.start();
            } else {
                orderListView.model = [];
                totalAmountLabel.text = "总计：￥0.00";
                countdownLabel.text = "";
                paymentCountdownTimer.stop();
            }
        }
    }

    Timer {
        id: paymentCountdownTimer
        interval: 1000
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            if (orderObject && orderObject.getStatusString() === "Pending") {
                let seconds = orderObject.getRemainingSeconds();
                countdownLabel.text = "剩余支付时间: " + formatRemainingTime(seconds);
                if (seconds <= 0) {
                    countdownLabel.text = "订单已超时";
                    confirmOrderButton.enabled = false;
                    // OrderManager.checkTimeoutOrders will handle backend cancellation
                }
            } else {
                countdownLabel.text = orderObject ? "订单状态: " + orderObject.getStatusString() : "无订单";
                this.stop();
            }
        }
    }

    function formatRemainingTime(totalSeconds) {
        if (totalSeconds <=0) return "00:00";
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

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
            interval: 3000
            onTriggered: {
                operationStatus.visible = false;
                if (operationStatus.success) {
                    orderPage.visible = false;
                    // mainContentPage.visible = true;
                    orderPaymentSuccess();
                } else {
                    orderPaymentFailed();
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
                id: goBackButton
                text: "返回购物车"
                onClicked: {
                    if(orderPage.orderObject &&
                       orderPage.orderObject.getStatusString() === "Pending" &&
                       orderPage.orderObject.getRemainingSeconds() > 0) {
                       confirmGoBackDialog.open();
                    } else {
                        goBack();
                    }
                }
            }
            Label {
                text: "确认订单"
                font.pixelSize: 20
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
            }
        }

        Label {
            id: countdownLabel
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: "blue"
        }

        ListView {
            id: orderListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: orderObject ? orderObject.getQmlItems() : []
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
                    id: totalAmountLabel
                    text: "总计：￥" + (orderObject ? orderObject.calculateTotal().toFixed(2) : "0.00")
                    font.bold: true
                    font.pixelSize: 16
                    Layout.alignment: Qt.AlignVCenter
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: confirmOrderButton
                    text: "确认下单"
                    enabled: orderObject && orderObject.getStatusString() === "Pending" && orderObject.getRemainingSeconds() > 0
                    onClicked: {
                        // if (orderData && global.username) {
                        //     const orderResult = OrderManager.createOrder(orderData.username, orderData.items);
                        //     global.balance = AuthManager.getBalance(global.username);
                        //     if (orderResult) {
                        //         // 清空购物车
                        //         ShoppingCart.loadShoppingCart(orderData.username);
                        //         operationStatus.text = "订单支付成功！";
                        //         operationStatus.color = "green";
                        //         operationStatus.success = true;
                        //     } else {
                        //         operationStatus.text = "订单支付失败，请检查余额或库存！";
                        //         operationStatus.color = "red";
                        //         operationStatus.success = false;
                        //     }
                        //     operationStatus.visible = true;
                        //     statusTimer.start();
                        //  }
                        if (orderObject && global.username) {
                            paymentCountdownTimer.stop(); // Stop countdown during payment attempt
                            const success = OrderManager.payOrder(orderObject, global.username);
                            global.balance = AuthManager.getBalance(global.username); // Update balance display

                            if (success) {
                                ShoppingCart.loadShoppingCart(global.username); // Clear/update cart
                                operationStatus.text = "订单支付成功！";
                                operationStatus.color = "green";
                                operationStatus.success = true;
                            } else {
                                operationStatus.text = "订单支付失败！"; // More specific error can come from backend if desired
                                operationStatus.color = "red";
                                operationStatus.success = false;
                                if (orderObject.getRemainingSeconds() > 0 && orderObject.getStatusString() === "Pending") {
                                     paymentCountdownTimer.start(); // Restart countdown if order still valid
                                } else {
                                     confirmOrderButton.enabled = false; // Disable if timed out or status changed
                                     countdownLabel.text = "订单状态: " + orderObject.getStatusString();
                                }
                            }
                            operationStatus.visible = true;
                            statusTimer.start(); // Show status message temporarily
                        }
                    }
                }
            }
            Dialog {
                id: confirmGoBackDialog
                title: "订单处于待支付状态，取消将会释放已冻结的库存，确认取消？"
                standardButtons: Dialog.NoButton
                modal: true
                parent: Overlay.overlay
                anchors.centerIn: parent

                width: Math.min(parent.width * 0.8, 400)

                // contentItem: Label {
                //     text: "订单处于待支付状态，取消将会释放已冻结的库存，确认取消？"
                //     wrapMode: Text.wordWarp
                //     padding: 10
                // }

                footer: DialogButtonBox {
                    spacing: 10
                    Button {
                        text: "确认"
                        onClicked: {
                            if (orderPage.orderObject) {
                                OrderManager.cancelOrder(orderPage.orderObject);
                                // orderPage.orderObject = null; // Let the goBack() lead to potential re-evaluation
                            }
                            confirmGoBackDialog.close();
                            orderPage.goBack(); // Signal to navigate back
                        }
                    }
                    Button {
                        text: "取消"
                        onClicked: {
                            confirmGoBackDialog.close();
                        }
                    }
                }
            }
        }
    }
}

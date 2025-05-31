// ShoppingCartPage.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ECommerce.Core 1.0

Item {
    id: cartPage
    visible: false
    anchors.fill: parent

    signal goBack()
    signal checkout(var orderData)

    // onVisibleChanged: if(visible) ShoppingCart.loadShoppingCart(global.username)

    Component.onCompleted: {
        if(global.username) {
            ShoppingCart.loadShoppingCart(global.username)
        }
    }

    Connections {
        target: cartPage
        function onVisibleChanged() {
            if (cartPage.visible && global.username) {
                ShoppingCart.loadShoppingCart(global.username);
                cartListView.updateCartItems();
            }
        }
    }

    Connections { // 监听全局用户变化，如果用户登出则可能需要清空或隐藏购物车
        target: global
        function onUsernameChanged() {
            if (!global.username && parent.visible) {
                goBack();
            } else if (global.username && parent.visible) {
                ShoppingCart.loadShoppingCart(global.username);
                cartListView.updateCartItems();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        anchors.margins: 10

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "返回商品列表"
                onClicked: goBack()
            }
            Label {
                text: "我的购物车"
                font.pixelSize: 20
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
            }
            // Item { Layout.preferredWidth: backButton.width }
        }


        // 商品列表
        ListView {
            id: cartListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ListModel {}

            Component.onCompleted: {
                updateCartItems();
            }

            function updateCartItems() {
                model.clear();
                const items = ShoppingCart.getCartItems();
                for(let i = 0; i < items.length; i++) {
                    model.append(items[i]);
                }
            }

            delegate: CartItemDelegate {}
            spacing: 10

            Connections {
                target: ShoppingCart
                function onCartChanged() {
                    cartListView.updateCartItems();
                    totalLabel.text = "总计：￥" + ShoppingCart.getTotalPrice().toFixed(2);
                }
            }
        }

        // 底部结算栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#e0e0e0"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 20
                Label {
                    id: totalLabel
                    text: "总计：￥" + ShoppingCart.getTotalPrice().toFixed(2)
                    font.bold: true
                    font.pixelSize: 16
                    Layout.alignment: Qt.AlignVCenter
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "生成订单"
                    enabled: cartListView.count > 0 && global.username
                    onClicked: {
                        // const orderData = {
                        //     username: global.username,
                        //     items: ShoppingCart.getCartItems(),
                        //     totalPrice: ShoppingCart.getTotalPrice()
                        // };
                        // checkout(orderData);
                        const itemsToOrder = ShoppingCart.getCartItems();
                        if (itemsToOrder.length === 0) {
                            console.log("Shopping cart is empty.");
                            return;
                        }
                        var newOrderObject = OrderManager.prepareOrder(global.username, itemsToOrder);
                        if (newOrderObject) {
                            checkout(newOrderObject);
                        } else {
                            console.log("Failed to prepare order. Check console for details (stock/product issues).");
                        }
                    }
                }
            }

            Connections {
                target: cartListView.model
                function onDataChanged() {
                    totalLabel.text = "总计：￥" + ShoppingCart.getTotalPrice()().toFixed(2);
                }
            }
        }
    }
}

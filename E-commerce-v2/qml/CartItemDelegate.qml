// CartItemDelegate.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ECommerce.Core 1.0

Rectangle {
    width: ListView.view ? ListView.view.width : 0
    height: 120
    color: index % 2 ? "#f0f0f0" : "white"

    property var itemData: modelData || model
    property string name: itemData.name
    property double price: itemData.price
    property string imagePath: itemData.imagePath
    property string description: itemData.description
    property int quantity: itemData.quantity

    Label {
       id: operationHint
       text: ""
       color: "red"
       font.pixelSize: 10
       visible: false
       anchors.bottom: parent.bottom
       anchors.right: parent.right
       anchors.rightMargin: 10
       Timer {
           id: hintTimer
           interval: 3000
           onTriggered: operationHint.visible = false
       }
   }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        // 商品图片
        Rectangle {
            Layout.preferredWidth: 80
            Layout.preferredHeight: 80
            color: "transparent"
            Image {
                anchors.fill: parent
                source: imagePath ? "file:///" + imagePath : "qrc:/images/placeholder.png"
                fillMode: Image.PreserveAspectFit
            }
        }

        // 商品信息
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            Label {
                text: name
                font.bold: true
                font.pixelSize: 16
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Label {
                text: description
                font.pixelSize: 12
                color: "#666"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                elide: Text.ElideRight
                maximumLineCount: 2
            }
            Label {
                text: "单价：￥" + price.toFixed(2)
                font.pixelSize: 14
                color: "green"
            }
            Label {
                text: "小计：￥" + (price * quantity).toFixed(2)
                font.pixelSize: 14
                font.bold: true
                color: "#d32f2f"
            }
        }

        // 数量调整
        ColumnLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 5
            Label {
                text: "数量"
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Button {
                    text: "-"
                    enabled: quantity > 1
                    onClicked: ShoppingCart.updateQuantityByName(name, quantity - 1)
                }
                TextField {
                    id: quantityField
                    text: quantity.toString()
                    validator: IntValidator { bottom: 1 }
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 40
                    onEditingFinished: {
                        let newVal = parseInt(text)
                        const success = ShoppingCart.updateQuantityByName(name, newVal)
                        if(!success) {
                            text = quantity
                            operationHint.text = "库存不足"
                            operationHint.visible = true;
                            hintTimer.start();
                        }
                    }
                }
                Button {
                    text: "+"
                    onClicked: {
                        const success = ShoppingCart.updateQuantityByName(name, quantity + 1);
                        if(!success) {
                            operationHint.text = "库存不足"
                            operationHint.visible = true
                            hintTimer.start();
                        }
                    }
                }
            }
            Button {
                text: "删除"
                Layout.alignment: Qt.AlignHCenter
                onClicked: ShoppingCart.removeItemByName(name)
            }
        }
    }
}

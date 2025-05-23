// 单个商品展示组件
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./" as Components
import ECommerce.Core 1.0

Rectangle
{
    width: ListView.view.width
    height: 120
    color: index % 2 ? "#f0f0f0" : "white"

    signal editRequested(var productData)

    Label {
        id: operationStatus
        color: "red"
        visible: false
        Layout.alignment: Qt.AlignHCenter
    }

    Timer {
        id: operationTimer
        interval: 2000
        onTriggered: operationStatus.visible = false
    }

    RowLayout
    {
        anchors.fill: parent
        spacing: 15
        anchors.margins: 10

        Image {
            source: model.imagePath ? "file:///" + model.imagePath : "qrc:/images/placeholder.png"
            Component.onCompleted: console.log("imagePath:", model.imagePath)
            Layout.preferredWidth: 80
            Layout.preferredHeight: 80
            fillMode: Image.PreserveAspectFit
        }

        // 商品信息
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            Label {
                text: model.name
                font.bold: true
                font.pixelSize: 16
                elide: Text.ElideRight // 文本过长时省略
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }
            Label {
                text: model.description
                font.pixelSize: 12
                color: "#666"
                wrapMode: Text.Wrap // 允许换行
                Layout.maximumWidth: 300 // 限制宽度
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }
            Label {
                text: "库存：" + model.stock
                font.pixelSize: 12
                color: "#444"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }
        }

        // 价格和按钮（右侧）
        ColumnLayout {
            Layout.alignment: Qt.AlignRight // 右对齐
            spacing: 5

            // 价格信息
            ColumnLayout {
                spacing: 2
                Label {
                    text: "原价：￥" + model.price.toFixed(2)
                    color: "gray"
                    font.strikeout: true
                    font.pixelSize: 12
                }
                Label {
                    text: "折扣：" + (model.discount * 100).toFixed(0) + "%"
                    color: "red"
                    visible: model.discount < 1.0
                    font.pixelSize: 12
                }
                Label {
                    text: "现价：￥" + model.currentPrice.toFixed(2)
                    color: "green"
                    font.pixelSize: 14
                    font.bold: true
                }
            }

            // 管理按钮
            Button {
                text: "管理"
                visible: global.isMerchant
                Layout.alignment: Qt.AlignRight // 按钮右对齐
                onClicked: {
                    editRequested({
                        id: index,
                        name: model.name,
                        description: model.description,
                        price: model.price,
                        stock: model.stock
                    });
                }
            }

            Button {
                text: "购买"
                visible: global.isConsumer
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    if(productModel.purchaseProduct(index, global.username)) {
                        global.balance = AuthManager.getBalance(global.username);
                        operationStatus.text = "购买成功！";
                    } else {
                        operationStatus.text = "购买失败，余额不足或库存不足";
                    }
                    operationStatus.visible = true
                    operationTimer.start();
                }
            }
        }
    }
}

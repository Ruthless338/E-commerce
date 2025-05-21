// 单个商品展示组件
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./" as Components

Rectangle
{
    width: ListView.view.width
    height: 100
    color: index % 2 ? "#f0f0f0" : "white"

    signal editRequested(var productData)

    RowLayout
    {
        anchors.fill: parent
        spacing: 15

        Image {
            source: model.imagePath || "qrc:/images/placeholder.png"
            Layout.preferredWidth: 80
            Layout.preferredHeight: 80
        }

        ColumnLayout
        {
            Label { text: model.name; font.bold: true }
            Label { text: model.description }
            Label { text: "库存：" + model.stock }
        }

        ColumnLayout
        {
            Label {
                text: "原价：￥" + model.basePrice.toFixed(2)
                color: "gray"
                font.strikeout: true // 删除线
            }
            Label {
                text: "折扣：" + (model.discount * 100).toFixed(0) + "%"
                color: "red"
                visible: model.discount < 1.0
            }
            Label {
                text: "现价：￥" + model.price.toFixed(2)
                color: "green"
            }

            // Button
            // {
            //     text: "加入购物车"
            //     visible: global.isConsumer
            //     enabled: model.stock>0
            //     onClicked: productModel.purchase(model.id)
            // }

            Button
            {
                text: "管理"
                visible: global.isMerchant
                onClicked: {
                    editRequested ({
                        id: index,
                        name: model.name,
                        description: model.description,
                        price: model.price,
                        stock: model.stock
                    });
                }
            }
        }
    }
}

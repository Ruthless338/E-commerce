import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    id: cartWindow
    ListView {
        model:cartModel
        delegate: RowLayout {
            Label { text: model.name }
            Label { text: "数量： "+model.quantity }
            Button {
                text: "删除"
                onClicked: cartModel.removeItem(model.id)
            }
        }
    }
    Button {
        text: "生成订单"
        onClicked: orderManager.CreateOrder(cartModel.items)
    }
}

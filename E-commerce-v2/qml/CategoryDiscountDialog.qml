// CategoryDiscountDialog.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: categoryDiscountDialog
    title: "设置类别折扣"
    width: 400
    height: 400
    visible: false
    anchors.centerIn: Overlay.overlay

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        ComboBox {
            id: categoryCombo
            model: ["图书", "服装", "食品"]
            Layout.fillWidth: true
        }

        TextField {
            id: discountInput
            placeholderText: "折扣率（0~100）"
            validator: IntValidator { bottom: 0; top: 100 }
            Layout.fillWidth: true
            inputMethodHints: Qt.ImhDigitsOnly
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10
            Button {
                text: "取消"
                onClicked: categoryDiscountDialog.visible = false
            }
            Button {
                text: "确定"
                onClicked: {
                    if (discountInput.text === "") {
                        console.error("折扣率不能为空");
                        return;
                    }
                    productModel.setCategoryDiscount(
                        categoryCombo.currentText,
                        parseFloat(discountInput.text) / 100
                    );
                    categoryDiscountDialog.visible = false
                }
            }
        }
    }
}

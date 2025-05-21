import QtQuick 2.15
import QtQuick.Dialogs
import QtQuick.Controls

Dialog {
    title: "设置类别折扣"
    ComboBox {
        id: categoryCombo
        model: ["图书", "服装", "食品"]
    }
    TextField {
        id: discountInput
        placeholderText: "折扣率（0~100）"
        validator: IntValidator { bottom: 0; top: 100 }
    }
    Button {
        text: "确定"
        onClicked: {
            productModel.setCategoryDiscount(
                categoryCombo.currentText,
                parseFloat(discountInput.text) / 100
            );
            this.close();
        }
    }
}

// qml/Auth/rechargeDialog.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ECommerce.Core 1.0

Dialog {
    id: rechargeDialog
    title: "账户充值"
    modal: true
    width: 350
    height: 200
    visible: false
    parent: Overlay.overlay
    anchors.centerIn: parent

    property string username: global.username

    Label {
        text: "当前余额：￥" + global.balance.toFixed(2)
        color: "green"
        Layout.alignment: Qt.AlignHCenter
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        // 充值金额
        TextField {
            id: tfAmount
            placeholderText: "请输入充值金额（元）"
            Layout.fillWidth: true
            validator: DoubleValidator {
                bottom: 0.01
                top: 10000
                decimals: 2
            }
            inputMethodHints: Qt.ImhFormattedNumbersOnly
        }

        // 错误提示
        Label {
            id: lblMessage
            color: "red"
            visible: text !== ""
        }

        // 按钮区
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10

            Button {
                text: "取消"
                onClicked: rechargeDialog.visible = false
                Material.background: "#e0e0e0"
                Material.foreground: "black"
            }

            Button {
                text: "确认充值"
                Material.background: "#2196F3"
                Material.foreground: "white"
                onClicked: {
                    if (validateInput()) {
                        const amount = parseFloat(tfAmount.text)
                        if(AuthManager.recharge(username, amount)) {
                            global.balance = AuthManager.getBalance(username)
                            lblMessage.text = "成功充值 ¥" + amount.toFixed(2)
                            Qt.callLater(rechargeDialog.close)
                        } else {
                            lblMessage.text = "充值失败"
                        }
                    }
                }
            }
        }
    }

    // 输入验证逻辑
    function validateInput() {
        if (tfAmount.text === "") {
            lblMessage.text = "请输入充值金额"
            return false
        }
        if (isNaN(parseFloat(tfAmount.text))) {
            lblMessage.text = "请输入有效的数字"
            return false
        }
        if (parseFloat(tfAmount.text) <= 0) {
            lblMessage.text = "金额必须大于0"
            return false
        }
        lblMessage.text = ""
        return true
    }
}

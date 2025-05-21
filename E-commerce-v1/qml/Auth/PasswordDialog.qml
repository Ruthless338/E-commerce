// qml/Auth/passwordDialog.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: passwordDialog
    title: "修改密码"
    modal: true
    width: 400
    height: 300
    visible: false
    parent: Overlay.overlay
    anchors.centerIn: parent

    property string username: global.username

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        // 当前密码
        TextField {
            id: tfCurrentPwd
            placeholderText: "请输入当前密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true
        }

        // 新密码
        TextField {
            id: tfNewPwd
            placeholderText: "请输入新密码（至少6位）"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            validator: RegularExpressionValidator {
                regularExpression: /.{6,}/
            }
        }

        // 确认新密码
        TextField {
            id: tfConfirmPwd
            placeholderText: "请再次输入新密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true
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
                onClicked: passwordDialog.visible = false
                Material.background: "#e0e0e0"
                Material.foreground: "black"
            }

            Button {
                text: "确认修改"
                Material.background: "#4CAF50"
                Material.foreground: "white"
                onClicked: {
                    if (validateInput()) {
                        // 调用C++修改密码接口
                        User.changePassword(username, tfCurrentPwd.text, tfNewPwd.text)
                        lblMessage.text = "密码修改成功"
                        Qt.callLater(passwordDialog.close)
                    }
                }
            }
        }
    }

    // 输入验证逻辑
    function validateInput() {
        if (tfCurrentPwd.text === "") {
            lblMessage.text = "请输入当前密码"
            return false
        }
        if (tfNewPwd.text.length < 6) {
            lblMessage.text = "新密码至少需要6位"
            return false
        }
        if (tfNewPwd.text !== tfConfirmPwd.text) {
            lblMessage.text = "两次输入的新密码不一致"
            return false
        }
        if (!User.verifyPassword(username, tfCurrentPwd.text)) {
            lblMessage.text = "当前密码错误"
            return false
        }
        lblMessage.text = ""
        return true
    }
}

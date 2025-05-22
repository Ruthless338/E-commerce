import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ECommerce.Core 1.0

Dialog
{
    id: loginRegisterDialog
    visible: true
    property string userType: "Consumer"
    signal loginSuccess(var userData)
    title: "电商平台 - 登录/注册"
    width: 500
    height: 450
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true

    ColumnLayout
    {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 15

        RowLayout {
            Label { text: "用户类型：" }
            RadioButton {
                text: "消费者"
                checked: true
                onClicked: loginRegisterDialog.userType = "Consumer"
            }
            RadioButton {
                text: "商家"
                onClicked: loginRegisterDialog.userType = "Merchant"
            }
        }
        RadioButton
        {
            id: rbLogin
            text: "登录"
            checked: true
        }
        RadioButton
        {
            id: rbRegister
            text: "注册"
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#cccccc"
        }

        TextField
        {
            id: tfUsername
            placeholderText: "用户名"
            Layout.fillWidth: true
        }

        TextField
        {
            id: tfPassword
            placeholderText: "密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            validator: RegularExpressionValidator {
                regularExpression: /.{6,}/
            }
        }

        Button
        {
            text: rbLogin.checked ? "登录" : "注册"
            Layout.alignment: Qt.AlignHCenter

            onClicked: {
                if(rbLogin.checked) {
                    // 调用C++登录验证逻辑
                    // console.log("用户类型：", userType);
                    // console.log("全局状态：", JSON.stringify(global));
                    if(User.verifyLogin(tfUsername.text, tfPassword.text)) {
                        lblMessage.text = "登录成功！";
                        loginRegisterDialog.loginSuccess({username: tfUsername.text, type: userType});
                        Qt.callLater(() => close());
                    } else {
                        lblMessage = "用户名或密码错误";
                    }
                } else {
                    // 调用C++注册逻辑
                    // console.log("用户类型：", userType);
                    // console.log("全局状态：", JSON.stringify(global));
                    if(tfPassword.text.length < 6) {
                        lblMessage.text = "密码至少需要6位";
                        return;
                    }
                    var res = User.registerUser(tfUsername.text, tfPassword.text, userType, 0.0);
                    if (res.success) {
                        lblMessage.text = "注册成功! 2秒后自动登录";
                        rbLogin.checked = true;
                        setTimeout(() => {
                            loginRegisterDialog.loginSuccess({username: tfUsername.text, type:userType});
                            close();
                        }, 2000);
                    } else {
                        lblMessage.text = res.error;
                    }
                }
            }
            background: Rectangle {
                color: parent.down ? "#4CAF50":"#8BC34A"
                radius: 5
            }
            contentItem: Text {
                text: parent.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Label
        {
            id: lblMessage
            color: "red"
            visible: text !== ""
        }
    }

}

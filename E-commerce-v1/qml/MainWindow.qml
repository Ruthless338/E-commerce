import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./Auth" as Auth
import "./" as Components

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 800
    height: 600
    title: "电商平台 - 主界面"

    // 登录窗口
    Component {
        id: loginDialogComponent
        Auth.LoginRegisterWindow {
            onLoginSuccess: (userData) => {
                // 更新全局状态
                global.username = userData.username;
                global.userType = userData.type;
                global.isConsumer = (userData.type === "Consumer");
                global.isMerchant = (userData.type === "Merchant");
                this.close(); // 关闭登录窗口
            }
        }
    }

    Components.AddProductDialog {
        id: addProductDialog
        visible: false
    }

    Auth.PasswordDialog {
        id: passwordDialog
        visible: false
    }

    Auth.RechargeDialog {
        id: rechargeDialog
        visible: false
    }

    Components.EditProductDialog {
        id: editProductDialog
        visible: false
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: global ?
                    (global.username ? "当前用户：" + global.username + " (" + global.userType + ")" : "未登录")
                    : "未登录"
            }
            Button {
                text: "登录/注册"
                visible: global ? !global.username : true
                onClicked: {
                    // 动态创建登录窗口
                    var dialog = loginDialogComponent.createObject(mainWindow);
                    dialog.open();
                }
            }
            Button {
                text: "退出登录"
                visible: global.username // 登录后显示
                onClicked: {
                    // 重置全局状态
                    global.username = "";
                    global.userType = "";
                    global.isConsumer = false;
                    global.isMerchant = false;
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // 搜索栏
        RowLayout {
            TextField {
                id: tfSearch
                placeholderText: "输入商品名称搜索..."
                Layout.fillWidth: true
            }
            Button {
                text: "搜索"
                onClicked: productModel.search(tfSearch.text)
            }
        }

        // 商品列表
        ListView {
            id: productList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: productModel
            delegate: Components.ProductDelegate {
                onEditRequested: (productData) => {
                    editProductDialog.productData = productData;
                    editProductDialog.visible = true;
                }
            }
            spacing: 5
            Connections {
                target: productModel
                onLayoutChanged: productList.positionViewBeginning()
            }
        }

        // 功能按钮区
        RowLayout {
            Button {
                text: "修改密码"
                visible: global ? global.isConsumer : false
                onClicked: passwordDialog.visible = true
            }
            Button {
                text: "修改密码"
                visible: global ? global.isMerchant : false
                onClicked: passwordDialog.visible = true
            }
            Button {
                text: "充值余额"
                visible: global ? global.isConsumer : false
                onClicked: rechargeDialog.visible = true
            }
            Button {
                text: "添加商品"
                visible: global ? global.isMerchant : false
                onClicked: addProductDialog.visible = true
            }
        }
    }
}

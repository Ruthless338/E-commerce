import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./Auth" as Auth
import "./" as Components
import ECommerce.Core 1.0

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
                global.balance = AuthManager.getBalance(userData.username);
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

    Components.CategoryDiscountDialog {
        id: categoryDiscountDialog
        visible: false
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: global ?
                    (global.username ? "当前用户：" + global.username + " (" + global.userType + ")\n余额:￥" + global.balance.toFixed(2) : "未登录")
                    : "未登录"
                wrapMode: Text.Wrap
                font.pixelSize: 14
                Layout.alignment: Qt.AlignLeft
            }

            // 占位Item推动按钮到右侧
            Item { Layout.fillWidth: true}

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

        // 搜索栏-关键词
        RowLayout {
            TextField {
                id: tfSearch
                placeholderText: "输入关键词..."
                Layout.fillWidth: true
            }
            ComboBox {
                id: searchType
                model: ["名称","描述"]
                Layout.preferredWidth: 100
            }
            Button {
                text: "搜索"
                onClicked: productModel.search(
                               tfSearch.text,
                               searchType.currentIndex,
                               minPrice.text,
                               maxPrice.text
                            )
            }
        }

        // 搜索栏-价格范围
        RowLayout {
            Label { text: "价格范围：" }
            TextField {
                id: minPrice
                placeholderText: "最低价"
                validator: DoubleValidator { bottom: 0 }
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onTextChanged: if(isNaN(parseFloat(text))) text = ""
                Layout.preferredWidth: 120
            }
            Label { text: "-" }
            TextField {
                id: maxPrice
                placeholderText: "最高价"
                validator: DoubleValidator { bottom: 0 }
                Layout.preferredWidth: 120
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
                    if(global.username) {
                        editProductDialog.productData = productData;
                        editProductDialog.visible = true;
                    } else {
                        console.log("请登录后再操作");
                    }
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
            visible: global.username // 登录后显示
            spacing: 10
            Button {
                text: "修改密码"
                visible: global.isConsumer || global.isMerchant
                onClicked: passwordDialog.visible = true
            }
            Button {
                text: "充值余额"
                visible: global.isConsumer
                onClicked: rechargeDialog.visible = true
            }
            Button {
                text: "添加商品"
                visible: global.isMerchant
                onClicked: addProductDialog.visible = true
            }
            Button {
                text: "设置折扣"
                visible: global.isMerchant
                onClicked: categoryDiscountDialog.visible = true
            }
        }
    }
}

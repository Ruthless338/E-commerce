import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Dialog {
    id: addProductDialog
    title: "添加商品"
    visible: false
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 500

    property string selectedImagePath: ""
    property bool isAdding: false

    Timer {
        id: closeTimer
        interval: 1500
        onTriggered: {
            addProductDialog.visible = false;
            isAdding = false;
        }
    }

    onVisibleChanged: {
        if(visible) {
            operationStatus.text = "";
            operationStatus.visible = false;
            pName.text = "";
            pDesc.text = "";
            pPrice.text = "";
            pStock.text = "";
            pCategory.currentIndex = 0;
            selectedImagePath = "";
        }
    }

    ColumnLayout {
        Label {
            id: operationStatus
            color: "green"
            visible: false
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: pName
            placeholderText: "商品名称"
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }
        TextField {
            id: pDesc
            placeholderText: "商品描述"
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }
        TextField {
            id: pPrice
            placeholderText: "价格"
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }
        TextField {
            id: pStock
            placeholderText: "库存"
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }
        ComboBox {
            id: pCategory;
            model: ["图书","服装","食品"]
            Layout.fillWidth: true
        }
        Button {
            text: "选择图片"
            onClicked: fileDialog.open()
            Layout.fillWidth: true
        }
        Label {
            text: "已选择图片:" + (addProductDialog.selectedImagePath || "无")
            Layout.fillWidth: true
        }
        FileDialog {
            id: fileDialog
            onAccepted: {
                if (fileDialog.selectedFiles.length > 0) {
                    // 获取原始路径并转换为纯本地路径字符串
                    var rawPath = fileDialog.selectedFiles[0];
                    var filePath = rawPath.toString()
                                  .replace(/^file:\/\//, "")  // 移除 URL 前缀
                                  .replace(/\\/g, "/");      // 统一为斜杠分隔符

                    // 提取文件名
                    var decodedPath = decodeURIComponent(filePath);
                    var fileName = decodedPath.split("/").pop();

                    // 构造目标路径
                    var destDir = "D:/Qt_projects/E-commerce/E-commerce-v1/images";
                    var destPath = destDir + "/" + fileName;

                    // 调用 C++ 函数复制文件
                    Qt.callLater(() => {
                        productModel.copyImage(decodedPath, destPath);
                    });
                    selectedImagePath = destPath;
                } else {
                    console.error("未选择文件");
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 15
            Layout.topMargin: 20
            Button {
                text: "提交"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 40
                onClicked: {
                    if (addProductDialog.selectedImagePath === "") {
                        console.error("未选择图片");
                        operationStatus.text = "请选择图片！";
                        operationStatus.color = "red";
                        operationStatus.visible = true;
                        return;
                    }

                    var merchantUsername = global.username;
                    if(!global.isMerchant || merchantUsername === "") {
                        operationStatus.text = "商家未登录，无法添加商品";
                        operationStatus.color = "red";
                        operationStatus.visible = true;
                        return ;
                    }

                    // 从对话框属性获取路径
                    var decodedPath = addProductDialog.selectedImagePath;
                    var fileName = decodedPath.split("/").pop();

                    var destDir = "D:/Qt_projects/E-commerce/E-commerce-v1/images";
                    var imagePath = destDir + "/" + fileName;

                    console.log("即将保存文件至：" + imagePath);

                    var res = productModel.addProduct(
                        pName.text,
                        pDesc.text,
                        parseFloat(pPrice.text),
                        parseInt(pStock.text),
                        pCategory.currentText,
                        merchantUsername,
                        imagePath
                    );
                    if(res) {
                        operationStatus.text = "商品添加成功！";
                        operationStatus.color = "green";
                        closeTimer.start();
                    } else {
                        operationStatus.text = "商品添加失败，请检查数据";
                        opetationStatus.color = "red";
                        isAdding = false;
                    }
                    operationStatus.visible = true;
                }
            }
            Button {
                text: "取消"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 40
                onClicked: {
                    addProductDialog.visible = false
                }
            }
        }
    }
}

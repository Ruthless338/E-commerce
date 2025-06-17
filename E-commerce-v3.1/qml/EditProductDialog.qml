// EditProductDialog.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtQuick.Dialogs

Dialog {
    id: editDialog
    title: "编辑商品"
    parent: Overlay.overlay
    anchors.centerIn: parent
    visible: false
    width: 500

    property var productData: ({})
    property string selectedImagePath: ""

    Timer {
        id: closeTimer
        interval: 1500
        onTriggered: {
            operationStatus.visible = false;
        }
    }

    onVisibleChanged: {
        if (visible) {
            operationStatus.text = "";
            operationStatus.visible = false;
            selectedImagePath = ""
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
            id: editName
            text: productData.name || ""
            placeholderText: "商品名称"
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }

        TextField {
            id: editDesc
            text: productData.description || ""
            placeholderText: "商品描述"
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }

        TextField {
            id: editPrice
            text: productData.price ? productData.price.toFixed(2) : ""
            placeholderText: "价格"
            validator: DoubleValidator { bottom: 0 }
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }

        TextField {
            id: editStock
            text: productData.stock || ""
            placeholderText: "库存"
            validator: IntValidator { bottom: 0 }
            Layout.fillWidth: true
            Layout.preferredWidth: 450
        }

        Button {
            text: "更换图片"
            onClicked: fileDialog.open()
            Layout.fillWidth: true
        }
        Label {
            text: "已选择图片:" + (editDialog.selectedImagePath || "无")
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
            Layout.topMargin: 20
            spacing: 15
            Layout.alignment: Qt.AlignHCenter
            Button {
                text: "保存"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 40
                onClicked: {
                    var imagePath = editDialog.selectedImagePath || productData.ImagePath;

                    // 仅提交用户修改的字段，未修改的保持原值
                    var res = productModel.updateProduct(
                        productData.id,
                        editName.text !== productData.name ? editName.text : productData.name,
                        editDesc.text !== productData.description ? editDesc.text : productData.description,
                        editPrice.text !== productData.price.toFixed(2) ? parseFloat(editPrice.text) : productData.price,
                        editStock.text !== productData.stock.toString() ? parseInt(editStock.text) : productData.stock,
                        imagePath
                    );

                    if (res) {
                        operationStatus.text = "✓ 商品信息已保存";
                        operationStatus.color = "#4CAF50";
                        closeTimer.start();
                    } else {
                        operationStatus.text = "❗ 保存失败，请检查数据";
                        operationStatus.color = "#f44336";
                    }
                    operationStatus.visible = true;

                    // 延迟关闭对话框
                    Qt.callLater(() => {
                        editDialog.visible = false;
                    });
                }
            }
            Button {
                text: "取消"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 40
                onClicked: editDialog.visible = false;
            }
        }
    }
}

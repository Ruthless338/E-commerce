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

    property string selectedImagePath: ""

    ColumnLayout {
        Label {
            id: operationStatus
            color: "green"
            visible: false
            Layout.alignment: Qt.AlignHCenter
        }

        TextField { id: pName; placeholderText: "商品名称" }
        TextField { id: pDesc; placeholderText: "商品描述" }
        TextField { id: pPrice; placeholderText: "价格" }
        TextField { id: pStock; placeholderText: "库存" }
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
            spacing: 10
            Button {
                text: "提交"
                onClicked: {
                    if (addProductDialog.selectedImagePath === "") {
                        console.error("未选择图片");
                        operationStatus.text = "请选择图片！";
                        operationStatus.color = "red";
                        operationStatus.visible = true;
                        return;
                    }

                    // 从对话框属性获取路径
                    var decodedPath = addProductDialog.selectedImagePath;
                    var fileName = decodedPath.split("/").pop();

                    // 修复目录拼写错误
                    var destDir = "D:/Qt_projects/E-commerce/E-commerce-v1/images";
                    var imagePath = destDir + "/" + fileName;

                    console.log("即将保存文件至：" + imagePath);
                    productModel.addProduct(
                        pName.text,
                        pDesc.text,
                        parseFloat(pPrice.text),
                        parseInt(pStock.text),
                        pCategory.currentText,
                        imagePath
                    );
                    operationStatus.text = "商品添加成功！";
                    operationStatus.color = "green";
                    Qt.setTimeout(() => addProductDialog.visible = false, 1500);
                }
            }
            Button {
                text: "取消"
                onClicked: {
                    addProductDialog.visible = false
                }
            }
        }
    }
}

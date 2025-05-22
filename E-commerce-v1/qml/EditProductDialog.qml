// EditProductDialog.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtQuick.Dialogs

Dialog {
    id: editDialog
    title: "编辑商品"
    anchors.centerIn: parent
    visible: false
    width: 400
    property var productData: ({})

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        TextField {
            id: editName
            text: productData.name || ""
            placeholderText: "商品名称"
            Layout.fillWidth: true
        }

        TextField {
            id: editDesc
            text: productData.description || ""
            placeholderText: "商品描述"
            Layout.fillWidth: true
        }

        TextField {
            id: editPrice
            text: productData.price ? productData.price.toFixed(2) : ""
            placeholderText: "价格"
            validator: DoubleValidator { bottom: 0 }
            Layout.fillWidth: true
        }

        TextField {
            id: editStock
            text: productData.stock || ""
            placeholderText: "库存"
            validator: IntValidator { bottom: 0 }
            Layout.fillWidth: true
        }

        Button {
            text: "更换图片"
            onClicked: fileDialog.open()
        }
        Image {
            source: productData.ImagePath || ""
            Layout.preferredHeight: 100
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
                } else {
                    console.error("未选择文件");
                }
            }
        }

        RowLayout {
            spacing: 10
            Button {
                text: "保存"
                onClicked: {
                    // 仅提交用户修改的字段，未修改的保持原值
                    productModel.updateProduct(
                        productData.id,
                        editName.text !== productData.name ? editName.text : productData.name,
                        editDesc.text !== productData.description ? editDesc.text : productData.description,
                        editPrice.text !== productData.price.toFixed(2) ? parseFloat(editPrice.text) : productData.price,
                        editStock.text !== productData.stock.toString() ? parseInt(editStock.text) : productData.stock
                    );
                    Toast.show("商品信息已更新", 1500, "#4CAF50");
                    editDialog.visible = false;
                }
            }
            Button {
                text: "取消"
                onClicked: editDialog.visible = false;
            }
        }
    }
}

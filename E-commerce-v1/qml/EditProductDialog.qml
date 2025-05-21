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
                if (fileDialog.fileUrl) {
                    var fileUrl = fileDialog.fileUrl.toString(); // 获取完整 URL
                    var fileName = fileUrl.split("/").pop(); // 提取文件名
                    var destPath = "file:///D:/Qt_projects/E-commerce/E-commerce-v1/images/" + fileName;
                    Qt.callLater(() => {
                        productModel.copyImage(fileUrl, destPath);
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

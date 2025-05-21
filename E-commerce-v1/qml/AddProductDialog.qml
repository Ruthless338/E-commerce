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

    ColumnLayout {
        TextField { id: pName; placeholderText: "商品名称" }
        TextField { id: pDesc; placeholderText: "商品描述" }
        TextField { id: pPrice; placeholderText: "价格" }
        TextField { id: pStock; placeholderText: "库存" }
        ComboBox {
            id: pCategory;
            model: ["图书","服装","食品"]
        }
        Button {
            text: "选择图片"
            onClicked: fileDialog.open()
        }
        Label {
            text: "已选择图片" + (fileDialog.fileUrl || "无")
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

        Button {
            text: "提交"
            onClicked: {
                productModel.addProduct(
                            pName.text,
                            pDesc.text,
                            parseFloat(pPrice.text),
                            parseInt(pStock.text),
                            pCategory.currentText);
                addProductDialog.visible = false;
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

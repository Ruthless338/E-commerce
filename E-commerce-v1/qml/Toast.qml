// Toast.qml
import QtQuick 2.15

Rectangle {
    id: toast
    width: 200
    height: 40
    color: "#4CAF50"
    radius: 5
    opacity: 0
    anchors.centerIn: Overlay.overlay

    Text {
        anchors.centerIn: parent
        color: "white"
        text: ""
    }

    function show(message, duration=2000, color="#4CAF50") {
        toast.color = color;
        toastText.text = message;
        opacity = 1;
        timer.interval = duration;
        timer.start();
    }

    Text { id: toastText }

    Timer {
        id: timer
        onTriggered: toast.opacity = 0
    }

    Behavior on opacity { NumberAnimation { duration: 300 } }
}

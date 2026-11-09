import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    property string freqText: (backend.littleFreq() / 1000).toFixed(0) + " / "
                             + (backend.bigFreq() / 1000).toFixed(0) + " MHz"

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: {
            freqText = (backend.littleFreq() / 1000).toFixed(0) + " / "
                     + (backend.bigFreq() / 1000).toFixed(0) + " MHz"
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.paddingSmall
        width: parent.width - 2 * Theme.paddingLarge

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("CPU Mode")
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.primaryColor
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: backend.mode
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.highlightColor
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: freqText
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }

    CoverActionList {
        CoverAction {
            iconSource: "image://theme/icon-m-refresh"
            onTriggered: {
                var modes = [ "powersave", "balanced", "performance" ]
                var idx = modes.indexOf(backend.mode)
                backend.setMode(modes[(idx + 1) % modes.length])
            }
        }
    }
}

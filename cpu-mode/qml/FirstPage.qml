import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property string currentMode: backend.mode

    property var littleHistory: []
    property var bigHistory: []
    readonly property int historyLength: 60
    readonly property real freqScale: 2500000.0
    readonly property int littleMax: backend.littleMax()
    readonly property int bigMax: backend.bigMax()

    Timer {
        interval: 500
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            var l = page.littleHistory.slice()
            var b = page.bigHistory.slice()
            l.push(backend.littleFreq())
            b.push(backend.bigFreq())
            if (l.length > page.historyLength) l.shift()
            if (b.length > page.historyLength) b.shift()
            page.littleHistory = l
            page.bigHistory = b
            chart.requestPaint()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("CPU Mode") }

            ComboBox {
                id: modeCombo
                label: qsTr("Power mode")
                description: qsTr("Changes CPU frequency scaling")

                menu: ContextMenu {
                    MenuItem { text: qsTr("Powersave") }
                    MenuItem { text: qsTr("Balanced") }
                    MenuItem { text: qsTr("Performance") }
                }

                currentIndex: currentMode === "powersave" ? 0
                            : currentMode === "performance" ? 2 : 1

                onCurrentIndexChanged: {
                    var modes = [ "powersave", "balanced", "performance" ]
                    var m = modes[currentIndex]
                    if (backend.setMode(m)) {
                        page.currentMode = m
                    }
                }
            }

            SectionHeader { text: qsTr("CPU frequency") }

            Canvas {
                id: chart
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: 220

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.clearRect(0, 0, width, height)

                    // background grid
                    ctx.strokeStyle = Theme.rgba(Theme.secondaryColor, 0.25)
                    ctx.lineWidth = 1
                    for (var g = 0; g <= 4; g++) {
                        var y = height * g / 4
                        ctx.beginPath()
                        ctx.moveTo(0, y)
                        ctx.lineTo(width, y)
                        ctx.stroke()
                    }

                    function drawLine(hist, color) {
                        if (hist.length < 2) return
                        ctx.strokeStyle = color
                        ctx.lineWidth = 2
                        ctx.beginPath()
                        for (var i = 0; i < hist.length; i++) {
                            var x = width * i / (page.historyLength - 1)
                            var y = height * (1.0 - Math.min(1.0, hist[i] / page.freqScale))
                            if (i === 0) ctx.moveTo(x, y)
                            else ctx.lineTo(x, y)
                        }
                        ctx.stroke()
                    }

                    drawLine(page.bigHistory, Theme.rgba(Theme.highlightColor, 0.55))
                    drawLine(page.littleHistory, Theme.highlightColor)
                }
            }

            Row {
                x: Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                Label {
                    text: qsTr("Little: %1 MHz").arg((backend.littleFreq() / 1000).toFixed(0))
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.highlightColor
                }
                Label {
                    text: qsTr("Big: %1 MHz").arg((backend.bigFreq() / 1000).toFixed(0))
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.rgba(Theme.highlightColor, 0.55)
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Limits: little %1 MHz, big %2 MHz")
                      .arg((littleMax / 1000).toFixed(0))
                      .arg((bigMax / 1000).toFixed(0))
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: currentMode === "powersave"
                      ? qsTr("CPU frequency is capped for battery life.")
                      : currentMode === "performance"
                        ? qsTr("CPU runs at full speed. Higher power consumption.")
                        : qsTr("Interactive scaling, small cores kept above 1.2 GHz.")
            }
        }
    }
}

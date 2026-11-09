#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QScopedPointer>
#include <QDebug>
#include <QUrl>
#include <QQmlError>
#include "backend.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    Backend backend;
    view->rootContext()->setContextProperty("backend", &backend);

    const QUrl src = SailfishApp::pathTo("qml/main.qml");
    qWarning() << "cpu-mode: loading" << src;

    QObject::connect(view.data(), &QQuickView::statusChanged,
        [](QQuickView::Status s) { qWarning() << "cpu-mode: status" << int(s); });

    view->setSource(src);
    for (const auto &e : view->errors())
        qWarning() << "cpu-mode: QML error:" << e.toString();

    view->show();
    return app->exec();
}

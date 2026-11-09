#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mode READ currentMode NOTIFY modeChanged)
public:
    explicit Backend(QObject *parent = nullptr);

    Q_INVOKABLE QString currentMode() const;
    Q_INVOKABLE bool setMode(const QString &mode);
    Q_INVOKABLE int littleFreq() const;
    Q_INVOKABLE int bigFreq() const;
    Q_INVOKABLE int littleMax() const;
    Q_INVOKABLE int bigMax() const;

signals:
    void modeChanged();
};

#endif

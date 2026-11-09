#include "backend.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>

static QString requestFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                        + QStringLiteral("/cpu-mode");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/mode");
}

static int readInt(const char *path)
{
    QFile f(QString::fromUtf8(path));
    if (f.open(QIODevice::ReadOnly))
        return QString::fromUtf8(f.readAll()).trimmed().toInt();
    return 0;
}

Backend::Backend(QObject *parent) : QObject(parent) {}

int Backend::littleFreq() const
{
    return readInt("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
}

int Backend::bigFreq() const
{
    return readInt("/sys/devices/system/cpu/cpu4/cpufreq/scaling_cur_freq");
}

int Backend::littleMax() const
{
    return readInt("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
}

int Backend::bigMax() const
{
    return readInt("/sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq");
}

QString Backend::currentMode() const
{
    QFile f(requestFile());
    if (f.open(QIODevice::ReadOnly)) {
        QString m = QString::fromUtf8(f.readAll()).trimmed();
        if (!m.isEmpty())
            return m;
    }
    return QStringLiteral("balanced");
}

bool Backend::setMode(const QString &mode)
{
    static const QStringList allowed = {"powersave", "balanced", "performance"};
    if (!allowed.contains(mode))
        return false;

    QFile f(requestFile());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(mode.toUtf8());
    f.write("\n");
    f.close();
    emit modeChanged();
    return true;
}

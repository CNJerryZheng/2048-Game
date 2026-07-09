#pragma once

#include <QString>
#include <QStringList>

namespace ModLoader {

struct LoadResult
{
    QStringList loaded;
    QStringList errors;
};

LoadResult loadModeMods(const QString &directory);

}

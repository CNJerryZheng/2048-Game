#include "mod_loader.h"

extern "C" {
#include "game_mode.h"
#include "utils.h"
}

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace ModLoader {

LoadResult loadModeMods(const QString &directory)
{
    LoadResult result;
    QDir dir(directory);
    dir.mkpath(".");
    const QStringList files = dir.entryList({"*.json"}, QDir::Files, QDir::Name);
    for (const QString &name : files)
    {
        QFile file(dir.filePath(name));
        if (!file.open(QIODevice::ReadOnly))
        {
            result.errors.append(name + "：无法读取");
            continue;
        }
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        const QJsonObject object = document.object();
        if (error.error != QJsonParseError::NoError || object.value("type").toString() != "mode")
        {
            result.errors.append(name + "：不是有效的模式 MOD");
            continue;
        }
        const QByteArray id = object.value("id").toString().toUtf8();
        const QByteArray displayName = object.value("name").toString().toUtf8();
        GameModeDefinition mode = {};
        utils_copy_string(mode.id, id.constData(), sizeof(mode.id));
        utils_copy_string(mode.display_name, displayName.constData(), sizeof(mode.display_name));
        mode.target_tile = object.value("targetTile").toInt(2048);
        mode.step_limit = object.value("stepLimit").toInt(0);
        mode.time_limit_seconds = object.value("timeLimitSeconds").toInt(0);
        mode.ranking_enabled = object.value("rankingEnabled").toBool(true);
        if (id.isEmpty() || displayName.isEmpty() || !game_mode_register(&mode))
        {
            result.errors.append(name + "：ID 无效或已经注册");
            continue;
        }
        result.loaded.append(QString::fromUtf8(mode.display_name));
    }
    return result;
}

}

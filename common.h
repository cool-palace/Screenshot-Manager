#ifndef COMMON_H
#define COMMON_H
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

QJsonObject json_object(const QString& filepath);
bool save_json(const QJsonObject& object, QFile& file);

#endif // COMMON_H

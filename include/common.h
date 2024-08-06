#ifndef COMMON_H
#define COMMON_H
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

QJsonObject json_object(const QString& filepath);
bool save_json(const QJsonObject& object, QFile& file);
QStringList word_forms(const QString&);
QString inflect(int, const QString&);
QList<QVector<int>> remove_duplicate_cycles(const QList<QVector<int>>& cycles);

#endif // COMMON_H

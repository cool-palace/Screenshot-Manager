#ifndef SERIES_INFO_H
#define SERIES_INFO_H
#include <QString>

struct SeriesInfo {
    SeriesInfo(int id, const QString& name, const QString& filepath, int size) : id(id), name(name), filepath(filepath), size(size) {};
    int id;
    QString name;
    QString filepath;
    int size;
};

#endif // SERIES_INFO_H

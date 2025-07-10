#ifndef HASHTAG_INFO_H
#define HASHTAG_INFO_H
#include <QString>

struct HashtagInfo {
    HashtagInfo() = default;
    HashtagInfo(int id, const QString& name, int rank, int count = 0) : id(id), name(name), rank(rank), count(count) {};
    int id;
    QString name;
    int rank;
    int count;
};
#endif // HASHTAG_INFO_H

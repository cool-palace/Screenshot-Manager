#ifndef LOCATIONS_H
#define LOCATIONS_H
#include <QJsonObject>
#include <QMap>
#include <QString>

enum Directories {
    JOURNALS,
    SCREENSHOTS,
    SCREENSHOTS_NEW,
    QUOTES,
    SUBS,
    SUBS_NEW,
    HASHTAGS,
    SERIES,
    LOGS_FILE,
    POLL_LOGS,
    PUBLIC_RECORDS,
    HIDDEN_RECORDS,
    LABELS,
    DATABASE,
    POSTS
};

class Locations
{
private:
    Locations() {};
public:
    static Locations& instance();
    Locations(const Locations&) = delete;
    Locations& operator=(const Locations&) = delete;
    void init(const QJsonObject& json_file);
    const QString operator[](Directories key) const;
private:
    QMap<Directories, QString> m_locations;
};

#endif // LOCATIONS_H

#ifndef QUERYFILTERS_H
#define QUERYFILTERS_H

#include <QDateTime>
#include <QList>
#include <QMap>
#include <set>

class HashtagFilter {
public:
    enum Code {
        BOTH    = -1,
        AMPTAG  = 0,
        HASHTAG = 1,
    };
    HashtagFilter() = default;
    HashtagFilter(int id, const QChar& c = ' '): m_id(id) {
        if (c == " ")      m_code = BOTH;
        else if (c == "&") m_code = AMPTAG;
        else if (c == "#") m_code = HASHTAG;
    }
    int id() const { return m_id; }
    Code code() const { return m_code; }
    QString sign() const {
        if (m_code == BOTH)         return "";
        else if (m_code == AMPTAG)  return "&";
        else if (m_code == HASHTAG) return "#";
        return "";
    }
    QString to_sql() const {
        if (m_code == HashtagFilter::BOTH)
            return QString("(%1, NULL)").arg(m_id);
        return QString("(%1, %2)").arg(m_id).arg(static_cast<int>(m_code));
    }
    bool operator<(const HashtagFilter& other) const {
        if (m_id != other.m_id)
            return m_id < other.m_id;
        return m_code < other.m_code;
    }
private:
    int m_id;
    Code m_code = BOTH;
};

struct PublicityFilter {
    bool enabled = false;
    bool hidden = false;
};

struct QuantityFilter {
    bool enabled = false;
    bool multiple = false;
};

struct LastUsedFilter {
    bool enabled = false;
    QDate date;
};

struct SeriesFilter {
    bool enabled = false;
    bool last_used = false;
    QDate date;
    std::set<int> included;
    std::set<int> excluded;
};

struct TextFilter {
    bool enabled = false;
    QString text;
};

struct HashtagsFilter {
    bool enabled = true;
    QMap<int, HashtagFilter> included;
    QMap<int, HashtagFilter> excluded;
};

struct QueryFilters {
private:
    QueryFilters() {}
public:
    static QueryFilters& instance() {
        static QueryFilters instance;
        return instance;
    }
    QueryFilters(const QueryFilters&) = delete;
    QueryFilters& operator=(const QueryFilters&) = delete;
    ~QueryFilters() {}
public:
    PublicityFilter publicity;
    QuantityFilter quantity;
    LastUsedFilter last_used;
    SeriesFilter series;
    HashtagsFilter hashtags;
    TextFilter text;
};

#endif // QUERYFILTERS_H

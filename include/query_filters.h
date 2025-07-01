#ifndef QUERYFILTERS_H
#define QUERYFILTERS_H

#include <QDateTime>
#include <QList>
#include <set>

class HashtagFilter {
    enum Code {
        BOTH    = -1,
        AMPTAG  = 0,
        HASHTAG = 1,
    };
public:
    HashtagFilter(const QString& tag, QChar c): m_tag(tag) {
        if (c == " ") m_code = BOTH;
        if (c == "&") m_code = AMPTAG;
        if (c == "#") m_code = HASHTAG;
    }
    QString to_sql() const {
        if (m_code == HashtagFilter::BOTH)
            return QString("('%1', NULL)").arg(m_tag);
        return QString("('%1', %2)").arg(m_tag).arg(static_cast<int>(m_code));
    }
private:
    QString m_tag;
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
    bool enabled = false;
    QList<HashtagFilter> included;
    QList<HashtagFilter> excluded;
};

struct QueryFilters {
    PublicityFilter publicity;
    QuantityFilter quantity;
    LastUsedFilter last_used;
    SeriesFilter series;
    HashtagsFilter hashtags;
    TextFilter text;
    uint32_t size = 1;
    bool ordered = false;
};

#endif // QUERYFILTERS_H

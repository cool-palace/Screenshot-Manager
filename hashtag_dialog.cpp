#include <hashtag_dialog.h>
#include <include/database.h>

QPair<int, int> HashtagDialog::m_rank_range = {-1, -1};

HashtagDialog::HashtagDialog(HashtagsFilter& filters,
                             const QueryFilters& query_filters,
                             const QMap<int, HashtagInfo>& hashtags,
                             QWidget *parent)
    : QDialog(parent), m_filters(filters), m_prev_filters(filters), m_qfilters(query_filters)
{
    setupUi(this);
    setWindowTitle("Выбор хэштегов");

    if (m_rank_range == QPair<int, int>{-1, -1})
        get_hashtag_ranks();

    sbMinRank->setRange(m_rank_range.first, m_rank_range.second);
    sbMaxRank->setRange(m_rank_range.first, m_rank_range.second);
    sbMaxRank->setValue(m_rank_range.second);

    connect(cbSorting, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HashtagDialog::sort_hashtags);

    for (const HashtagInfo& hashtag : hashtags) {
        HashtagButtonDB* button = new HashtagButtonDB(hashtag, this);
        m_hashtags.insert(hashtag.id, button);
        grlHashtags->addWidget(button);
        // Активируем фильтры
        if (filters.included.contains(hashtag.id)) {
            const HashtagFilter& filter = filters.included.value(hashtag.id);
            button->set_filter(true, filter);
        } else if (filters.excluded.contains(hashtag.id)) {
            const HashtagFilter& filter = filters.excluded.value(hashtag.id);
            button->set_filter(false, filter);
        } else button->setEnabled(hashtag.count);
        // Подключаем сигнал
        connect(button, &HashtagButtonDB::filterEvent, this, &HashtagDialog::filter_event);
    }
    setWindowState(Qt::WindowMaximized);
}

void HashtagDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    lay_hashtags(event);
}

void HashtagDialog::filter_event(const HashtagFilterType type, const int id) {
    if (type & EXCLUDE && m_filters.included.contains(id))
        return;
    if (!(type & EXCLUDE) && m_filters.excluded.contains(id))
        return;
    QChar c = type & ANY_TAG ? ' ' : type & HASHTAG ? '#' : '&';
    HashtagFilter filter(id, c);
    QMap<int, HashtagFilter>& filters_map = type & EXCLUDE ? m_filters.excluded : m_filters.included;
    if (filters_map.contains(id))
        filters_map.remove(id);
    else
        filters_map.insert(id, filter);

    for (HashtagButtonDB* button : m_hashtags) {
        if (!m_filters.excluded.contains(button->id())) {
            button->setDisabled(true);
            button->setFlat(true);
        }
    }
    QSqlQuery query;
    Database::instance().count_hashtags(query, m_qfilters);
    while (query.next()) {
        int id = query.value("id").toInt();
        int count = query.value("count").toInt();
        m_hashtags[id]->set_count(count);
    }
}

void HashtagDialog::set_filter_buttons() {

}

void HashtagDialog::get_hashtag_ranks() {
    QSqlQuery query;
    Database::instance().select_hashtag_ranks(query);
    if (query.next()) {
        m_rank_range.first = query.value("min_rank").toInt();
        m_rank_range.second = query.value("max_rank").toInt();
    } else {
        m_rank_range.first = 0;
        m_rank_range.second = 50;
    }
}

void HashtagDialog::sort_hashtags() {
    int button_width = 130;
    int columns = std::max(1, (saHashtags->width() - 100) / button_width);
    m_current_tag_columns = columns;
    QLayoutItem* child;
    while ((child = grlHashtags->takeAt(0))) {
        // Clearing items from the grid
        child->widget()->hide();
    }
    using Iterator = QMap<int, HashtagButtonDB*>::const_iterator;
    QVector<Iterator> iters;
    iters.reserve(m_hashtags.size());
    for (auto it = m_hashtags.cbegin(); it != m_hashtags.cend(); ++it) {
        int rank = it.value()->rank();
        if (rank >= sbMinRank->value() && rank <= sbMaxRank->value()) {
            iters.push_back(it);
        }
    }
    static auto ranked_sort = [](const Iterator& it1, const Iterator& it2){
        if (it1.value()->rank() != it2.value()->rank()) {
            return it1.value()->rank() < it2.value()->rank();
        }
        return it1.key() < it2.key();
    };
    static auto popular_sort = [this](const Iterator& it1, const Iterator& it2){
        return it1.value()->count() > it2.value()->count();
    };
//    static auto alphabet_sort = [](const Iterator& it1, const Iterator& it2){
//        return it1->text() < it2->text();
//    };
    if (cbSorting->currentIndex() == 1) {
        std::sort(iters.begin(), iters.end(), popular_sort);
    } else if (cbSorting->currentIndex() == 2) {
        std::sort(iters.begin(), iters.end(), ranked_sort);
    }
    int i = 0;
    for (const auto iter : iters) {
        const auto button = iter.value();
        grlHashtags->addWidget(button, i / columns, i % columns);
        button->show();
        ++i;
    }
}

void HashtagDialog::lay_hashtags(QResizeEvent *event) {
    static int button_width = 130;
    int columns = std::max(1, (saHashtags->width() - 100) / button_width);
    if (columns == m_current_tag_columns) return;
    m_current_tag_columns = columns;
    QVector<QLayoutItem*> items;
    items.resize(grlHashtags->children().size());
    QLayoutItem* child;
    while ((child = grlHashtags->takeAt(0))) {
        items.push_back(child);
    }
    int i = 0;
    for (const auto item : items) {
        grlHashtags->addItem(item, i / columns, i % columns);
        ++i;
    }
}

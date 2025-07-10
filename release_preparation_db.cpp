#include "release_preparation_db.h"
#include <include/common.h>
#include <include/database.h>
#include "series_dialog.h"
#include "hashtag_dialog.h"
#include "record_preview_db.h"
#include <QSqlQuery>

ReleasePreparationDB::ReleasePreparationDB(QWidget *parent) : QWidget(parent) {
    setupUi(this);

    connect(&VK_Manager::instance(), &VK_Manager::posted_successfully, this, &ReleasePreparationDB::posting_success);
    connect(&VK_Manager::instance(), &VK_Manager::post_failed, this, &ReleasePreparationDB::posting_fail);
    connect(pbGenerate, &QPushButton::clicked, this, &ReleasePreparationDB::generate_button);
    connect(pbPost, &QPushButton::clicked, this, &ReleasePreparationDB::post_button);
    connect(pbTextSearch, &QPushButton::clicked, this, &ReleasePreparationDB::text_filter_changed);
    connect(pbTextReset, &QPushButton::clicked, this, &ReleasePreparationDB::text_filter_reset);
    connect(pbSeriesDialog, &QPushButton::clicked, this, &ReleasePreparationDB::series_dialog);
    connect(pbHashtagsDialog, &QPushButton::clicked, this, &ReleasePreparationDB::hashtag_dialog);

    connect(grpPublicity, &QGroupBox::clicked, this, &ReleasePreparationDB::publicity_filter_changed);
    connect(grpQuantity,  &QGroupBox::clicked, this, &ReleasePreparationDB::quantity_filter_changed);
    connect(grpLastUsed,  &QGroupBox::clicked, this, &ReleasePreparationDB::last_used_filter_changed);
    connect(grpSeries,    &QGroupBox::clicked, this, &ReleasePreparationDB::series_filter_changed);

    connect(rbSeriesLastUsed, &QRadioButton::toggled, this, &ReleasePreparationDB::series_filter_changed);

    connect(cbPublicity,    QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReleasePreparationDB::publicity_filter_changed);
    connect(cbQuantity,     QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReleasePreparationDB::quantity_filter_changed);

    connect(sbLastUsedDays,   QOverload<int>::of(&QSpinBox::valueChanged), this, &ReleasePreparationDB::last_used_filter_changed);
    connect(sbSeriesLastUsed, QOverload<int>::of(&QSpinBox::valueChanged), this, &ReleasePreparationDB::series_filter_changed);

}

void ReleasePreparationDB::start() {
    deDate->setMinimumDate(QDate::currentDate());
    deDate->setDate(QTime::currentTime() < QTime(3,0)
                          ? QDate::currentDate()
                          : QDate::currentDate().addDays(1));
    bool weekend = deDate->date().dayOfWeek() > 5;
    teTime->setTime(weekend ? QTime(10,0) : QTime(8,0));
    if (weekend) sbSize->setValue(6);

    QSqlQuery query;
    Database::instance().count_series(query);
    if (query.next()) {
        int value = query.value("count").toInt();
        m_series_size = value;
    }
    Database::instance().select_hashtag_info(query);
    while (query.next()) {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();
        int rank = query.value("rank").toInt();
        m_hashtag_info.insert(id, HashtagInfo(id, name, rank));
    }
    set_enabled(true);

    publicity_filter_changed();
    last_used_filter_changed();
    series_filter_changed();
}

void ReleasePreparationDB::set_enabled(bool enable) {
    deDate->setEnabled(enable);
    pbGenerate->setEnabled(enable);
    teInterval->setEnabled(enable);
    teTime->setEnabled(enable);
    pbPost->setEnabled(enable);
    sbSize->setEnabled(enable);
}

void ReleasePreparationDB::publicity_filter_changed() {
    QueryFilters::instance().publicity.enabled = grpPublicity->isChecked();
    QueryFilters::instance().publicity.hidden = cbPublicity->currentIndex() > 0;
    update_results();
}

void ReleasePreparationDB::quantity_filter_changed() {
    QueryFilters::instance().quantity.enabled = grpQuantity->isChecked();
    QueryFilters::instance().quantity.multiple = cbQuantity->currentIndex() > 0;
    update_results();
}

void ReleasePreparationDB::last_used_filter_changed() {
    sbLastUsedDays->setEnabled(grpLastUsed->isChecked());
    QueryFilters::instance().last_used.enabled = grpLastUsed->isChecked();
    QueryFilters::instance().last_used.date = deDate->date().addDays(-sbLastUsedDays->value());
    update_results();
}

void ReleasePreparationDB::series_filter_changed() {
    QDate date = deDate->date().addDays(-sbSeriesLastUsed->value());
    // Обновляем фильтр
    const bool enabled = grpSeries->isChecked();
    QueryFilters::instance().series.enabled = enabled;
    QueryFilters::instance().series.last_used = rbSeriesLastUsed->isChecked();
    QueryFilters::instance().series.date = date;

    // Если задано исключение по дате
    if (QueryFilters::instance().series.last_used) {
        // Обновляем списки
        QueryFilters::instance().series.excluded.clear();
        QueryFilters::instance().series.included.clear();
        QSqlQuery query;
        Database::instance().select_excluded_series_ids(query, date);
        while (query.next()) {
            int id = query.value("id").toInt();
            QueryFilters::instance().series.excluded.insert(id);
        }
        for (int id  = 1; id <= m_series_size; ++id)
            if (!QueryFilters::instance().series.excluded.count(id))
                QueryFilters::instance().series.included.insert(id);
    }
    // Обновляем состояние виджетов
    sbSeriesLastUsed->setEnabled(enabled && QueryFilters::instance().series.last_used);
    pbSeriesDialog->setEnabled(enabled && !QueryFilters::instance().series.last_used);
    const int days = sbSeriesLastUsed->value();
    sbSeriesLastUsed->setSuffix(" " + inflect(days, "дней"));
    lblSeriesCount->setText(QString("%1 из %2").arg(QueryFilters::instance().series.included.size()).arg(m_series_size));

    update_results();
}

void ReleasePreparationDB::hashtags_filter_changed() {
    lblHashtags->setText(hashtag_filters());
    update_results();
}

void ReleasePreparationDB::text_filter_changed() {
    const bool enabled = leSearchBar->text().size() > 2;
    QueryFilters::instance().text.enabled = enabled;
    QueryFilters::instance().text.text = leSearchBar->text().replace("'", "''");
    update_results();
}

void ReleasePreparationDB::text_filter_reset() {
    leSearchBar->clear();
    text_filter_changed();
}

void ReleasePreparationDB::update_results() {
    int count = Database::instance().count_records(QueryFilters::instance());
    lblRecords->setText(QString("Найдено результатов: %1").arg(count));
}

void ReleasePreparationDB::get_series_info() {
    QSqlQuery query;
    Database::instance().select_series_info(query);
    while (query.next()) {
        int id = query.value("id").toInt();
        QString name = query.value("series_name").toString();
        int count = query.value("record_count").toInt();
        QString filepath = query.value("filepath").toString();
        m_series_info.append(SeriesInfo(id, name, filepath, count));
    }
}

void ReleasePreparationDB::series_dialog() {
    if (m_series_info.isEmpty())
        get_series_info();

    SeriesDialog dialog(QueryFilters::instance().series.included, m_series_info, this);
    if (dialog.exec() == QDialog::Accepted) {
        const auto results = dialog.results();
        QueryFilters::instance().series.included = std::move(results.first);
        QueryFilters::instance().series.excluded = std::move(results.second);
    }
    series_filter_changed();
}

void ReleasePreparationDB::hashtag_dialog() {
    update_hashtag_count();

    HashtagDialog dialog(QueryFilters::instance().hashtags, QueryFilters::instance(), m_hashtag_info);
    if (dialog.exec() == QDialog::Rejected) {
        QueryFilters::instance().hashtags = dialog.old_results();
    }
    hashtags_filter_changed();
}

void ReleasePreparationDB::update_hashtag_count() {
    QSqlQuery query;
    Database::instance().count_hashtags(query, QueryFilters::instance());
    while (query.next()) {
        int id = query.value("id").toInt();
        int count = query.value("count").toInt();
        m_hashtag_info.find(id)->count = count;
    }
}

bool ReleasePreparationDB::open_database() {
    return 1;
}

void ReleasePreparationDB::generate_button() {
    clear_grid(grlPreview);
    for (RecordPreviewDB* record : m_selected_records)
        delete record;
    m_selected_records.clear();
    generate_release();
}

void ReleasePreparationDB::generate_release() {
    QDateTime time = QDateTime(deDate->date(), teTime->time(), Qt::LocalTime);
    QSqlQuery query;
    Database::instance().select_records(query, QueryFilters::instance(), true, sbSize->value());
    RecordPreviewDB* previous = nullptr;
    while (query.next()) {
        RecordPreviewInfo record(query);
        RecordPreviewDB* record_preview = new RecordPreviewDB(record, time, this);
        m_selected_records.append(record_preview);
        if (previous) {
            previous->set_next(record_preview);
            record_preview->set_prev(previous);
        }
        previous = record_preview;
        grlPreview->addWidget(record_preview);
        time = time.addSecs(teInterval->time().hour()*3600 + teInterval->time().minute()*60);
    }
}

void ReleasePreparationDB::post_button() {

}

void ReleasePreparationDB::posting_success(int, int) {

}

void ReleasePreparationDB::posting_fail(int, const QString &) {

}

QString ReleasePreparationDB::hashtag_filters() {
    QStringList result;
    for (const HashtagFilter& filter : QueryFilters::instance().hashtags.included)
        result += filter.sign() + m_hashtag_info.value(filter.id()).name;
    for (const HashtagFilter& filter : QueryFilters::instance().hashtags.excluded)
        result += "-" + filter.sign() + m_hashtag_info.value(filter.id()).name;
    if (!result.isEmpty())
        return result.join(", ");
    return "Хэштеги не выбраны";
}


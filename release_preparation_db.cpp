#include "release_preparation_db.h"
#include <include/common.h>
#include <include/database.h>
#include "series_dialog.h"
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
    Database::instance().select_series_ids(query);
    while (query.next()) {
        int value = query.value("id").toInt();
        m_series.insert(value);
    }
    set_enabled(true);

//    grpLastUsed->setChecked(true);
    //    grpSeries->setChecked(true);
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
    m_filters.publicity.enabled = grpPublicity->isChecked();
    m_filters.publicity.hidden = cbPublicity->currentIndex() > 0;
    update_results();
}

void ReleasePreparationDB::quantity_filter_changed() {
    m_filters.quantity.enabled = grpQuantity->isChecked();
    m_filters.quantity.multiple = cbQuantity->currentIndex() > 0;
    update_results();
}

void ReleasePreparationDB::last_used_filter_changed() {
    sbLastUsedDays->setEnabled(grpLastUsed->isChecked());
    m_filters.last_used.enabled = grpLastUsed->isChecked();
    m_filters.last_used.date = deDate->date().addDays(-sbLastUsedDays->value());
    update_results();
}

void ReleasePreparationDB::series_filter_changed() {
    QDate date = deDate->date().addDays(-sbSeriesLastUsed->value());
    // Обновляем фильтр
    const bool enabled = grpSeries->isChecked();
    m_filters.series.enabled = enabled;
    m_filters.series.last_used = rbSeriesLastUsed->isChecked();
    m_filters.series.date = date;

    // Если задано исключение по дате
    if (m_filters.series.last_used) {
        // Обновляем списки
        m_filters.series.excluded.clear();
        m_filters.series.included.clear();
        QSqlQuery query;
        Database::instance().select_excluded_series_ids(query, date);
        while (query.next()) {
            int id = query.value("id").toInt();
            m_filters.series.excluded.insert(id);
        }
        for (const int id : m_series)
            if (!m_filters.series.excluded.count(id))
                m_filters.series.included.insert(id);
    }
    // Обновляем состояние виджетов
    sbSeriesLastUsed->setEnabled(enabled && m_filters.series.last_used);
    pbSeriesDialog->setEnabled(enabled && !m_filters.series.last_used);
    const int days = sbSeriesLastUsed->value();
    sbSeriesLastUsed->setSuffix(" " + inflect(days, "дней"));
    lblSeriesCount->setText(QString("%1 из %2").arg(m_filters.series.included.size()).arg(m_series.size()));

    update_results();
}

void ReleasePreparationDB::hashtags_filter_changed() {
    const bool enabled = grpHashtags->isChecked();
    m_filters.hashtags.enabled = enabled;
    lblHashtags->setEnabled(enabled);
    pbHashtagsDialog->setEnabled(enabled);
    update_results();
}

void ReleasePreparationDB::text_filter_changed() {
    const bool enabled = leSearchBar->text().size() > 2;
    m_filters.text.enabled = enabled;
    m_filters.text.text = leSearchBar->text().replace("'", "''");
    update_results();
}

void ReleasePreparationDB::text_filter_reset() {
    leSearchBar->clear();
    text_filter_changed();
}

void ReleasePreparationDB::update_results() {
    m_filters.ordered = true;
    m_filters.size = 0;
    QSqlQuery query;
    Database::instance().count_records(query, m_filters);
    while (query.next()) {
        int count = query.value("count").toInt();
        lblRecords->setText(QString("Найдено результатов: %1").arg(count));
    }
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

    SeriesDialog dialog(m_filters.series.included, m_series_info, this);
    if (dialog.exec() == QDialog::Accepted) {
        const auto results = dialog.results();
        m_filters.series.included = std::move(results.first);
        m_filters.series.excluded = std::move(results.second);
    }
    series_filter_changed();
}

bool ReleasePreparationDB::open_database() {
    return 1;
}

void ReleasePreparationDB::generate_button() {

}

void ReleasePreparationDB::generate_release() {

}

void ReleasePreparationDB::post_button() {

}

void ReleasePreparationDB::posting_success(int, int) {

}

void ReleasePreparationDB::posting_fail(int, const QString &) {

}


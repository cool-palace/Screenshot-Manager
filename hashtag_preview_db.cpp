#include "hashtag_preview_db.h"
#include "records_dialog.h"
#include <include/common.h>

HashtagPreviewDB::HashtagPreviewDB(const HashtagInfo& info, QWidget *parent)
    : QWidget(parent), m_info(info)
{
    setupUi(this);
    update();
    connect(pbReroll,  &QPushButton::clicked, this, &HashtagPreviewDB::reroll);
    connect(pbRecords, &QPushButton::clicked, this, &HashtagPreviewDB::check_records);
    connect(pbChoose,  &QPushButton::clicked, this, &HashtagPreviewDB::choose);
}

void HashtagPreviewDB::set_hashtag(const HashtagInfo &info) {
    m_info = info;
    update();
    emit changed();
}

void HashtagPreviewDB::update() {
    lblCount->setText(QString().setNum(m_info.count));
    lblName->setText(m_info.emoji + ' ' + m_info.name);
    leDescription->setText(m_info.description);

    auto font = lblLogs->font();
    if (m_info.date.isValid()) {
        int days = m_info.date.daysTo(QDateTime::currentDateTime());
        lblLogs->setText(QString("Публиковалось %1 %2 назад").arg(days).arg(inflect(days, "дней")));
        font.setBold(true);
        font.setItalic(false);
    } else {
        lblLogs->setText(QString("Раньше не публиковалось"));
        font.setBold(false);
        font.setItalic(true);
    }
    lblLogs->setFont(font);
}

void HashtagPreviewDB::reroll() {
    QSqlQuery query;
    Database::instance().select_hashtag_info(query, QueryFilters::instance(), 1);
    if (query.next())
        set_hashtag(HashtagInfo(query));
}

void HashtagPreviewDB::choose() {
    emit dialog_start();
}

void HashtagPreviewDB::check_records() {
    QueryFilters::instance().hashtags.enabled = true;
    QueryFilters::instance().hashtags.included.insert(m_info.id, HashtagFilter(m_info.id));
    RecordsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {    }
    QueryFilters::instance().hashtags.included.remove(m_info.id);
    QueryFilters::instance().hashtags.enabled = false;
}

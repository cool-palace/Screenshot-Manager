#include "record_item_db.h"

RecordItemDB::RecordItemDB(const RecordPreviewInfo & record, QWidget *parent) :
    QWidget(parent), m_info(std::move(record))
{
    setupUi(this);
    QFont font("Segoe UI", 10);
    lblNumber->setFont(font);
    lblText->setFont(font);
    lblLogs->setFont(font);
    saImages->setMinimumSize(180, 120);
    pbTime->hide();
    pbReroll->hide();
    pbSearch->hide();
    pbNumber->hide();
    pbSwitchUp->hide();
    pbSwitchDown->hide();
    update();
    connect(pbSelect, &QPushButton::clicked, [this]() { emit selected(m_info); } );
}

void RecordItemDB::mouseDoubleClickEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton ) {
        emit selected(m_info);
    }
}

void RecordItemDB::set_record(RecordPreviewInfo && record) {
    m_info = record;
    update();
}

void RecordItemDB::update() {
    lblNumber->setText(QString().setNum(m_info.id));
    update_text();
    update_images();
    update_log_info();
}

void RecordItemDB::update_log_info() {
    auto font = lblLogs->font();
    if (m_info.date.isValid()) {
        int days = m_info.date.daysTo(QDateTime::currentDateTime());
        lblLogs->setText(QString("\tПубликовалось %1 %2 назад").arg(days).arg(inflect(days, "дней")));
        font.setBold(true);
        font.setItalic(false);
    } else {
        lblLogs->setText(QString("\tРаньше не публиковалось"));
        font.setBold(false);
        font.setItalic(true);
    }
    lblLogs->setFont(font);
}

void RecordItemDB::update_images() {
    static QSize size = QSize(160, 90);
    // Removing images from the end if images.size() > links.size()
    for (int j = m_images.size(); j > m_info.links.size(); --j) {
        m_images.pop_back();
    }
    for (int i = 0; i < m_info.links.size(); ++i) {
        if (m_images.size() <= i) {
            // Adding new image
            m_images.push_back(QSharedPointer<RecordFrame>(new RecordFrame(m_info.links[i], size)));
            hlImages->addWidget(m_images.back().get());
        } else {
            m_images[i].data()->set_image(m_info.links[i], size);
        }
    }
    hlImages->setAlignment(m_info.links.size() > 1 ? Qt::AlignLeft : Qt::AlignHCenter);
}

void RecordItemDB::update_text() {
    lblText->setText(m_info.quote + '\n' + m_info.hashtags);
}

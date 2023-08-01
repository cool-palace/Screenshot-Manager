#include "record.h"

QJsonObject Record::to_json() const {
    QJsonObject current_record;
    QJsonArray pic_array, id_array, link_array;
    for (int i = 0; i < pics.size(); ++i) {
        pic_array.push_back(pics[i]);
        id_array.push_back(ids[i]);
        link_array.push_back(links[i]);
    }
    current_record["filenames"] = pic_array;
    current_record["photo_ids"] = id_array;
    current_record["links"] = link_array;
    current_record["caption"] = quote;
    current_record["public"] = is_public;
    return current_record;
}

RecordBase::RecordBase(const Record& record, int index) :
    QWidget(),
    index(index)
{
    update_text(record.quote);
    text.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    text.setMaximumHeight(120);
    text.setWordWrap(true);
    auto font = text.font();
    font.setPointSize(12);
    text.setFont(font);
    text.hide();
    number.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    number.setMinimumWidth(20);
    number.setFont(font);
    number.setText(QString().setNum(index + 1));
    number.hide();
    layout.setContentsMargins(0,0,0,0);
    setLayout(&layout);
}

RecordItem::RecordItem(const Record& record, int index, const QString& path) :
    RecordBase(record, index)
{
    QtConcurrent::run(this, &RecordItem::load_thumbmnail, path + record.pics[0]);
    box.setText("Кадров: " + QString().setNum(record.pics.size()));
    box.setMinimumHeight(15);
    box.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    box.setChecked(record.is_public);
    box.setEnabled(false);
    box.hide();
    layout.addWidget(&number,0,0);
    layout.addWidget(&image,0,1);
    layout.addWidget(&text,0,2);
    layout.addWidget(&box,1,1);
}

void RecordItem::include_log_info(int timestamp) {
    QDateTime last = QDateTime::fromSecsSinceEpoch(timestamp, Qt::LocalTime);
    auto num = number.text();
    number.setText(num + QString("\n(%1)").arg(last.daysTo(QDateTime::currentDateTime())));
}


RecordItem::RecordItem(const QString& quote, int index) :
    RecordBase(Record(quote), index)
{
    layout.addWidget(&number,0,0);
    layout.addWidget(&text,0,1);
}

void RecordBase::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton ) {
        emit selected(index);
    }
}

void RecordItem::set_gallery_view() {
    text.hide();
    number.hide();
    box.show();
    show();
}

void RecordItem::set_list_view() {
    box.hide();
    number.show();
    text.show();
    show();
}

void RecordPreview::set_list_view() {
    number.show();
    text.show();
    log_info.show();
    show();
}

void RecordBase::update_text(const QString& caption) {
    QRegularExpression regex("(.*?)?([#&])(.*)?$");
    auto i = regex.globalMatch(caption);
    if (i.hasNext()) {
        auto match = i.peekNext();
        text.setText(match.captured(1) + '\n' + match.captured(2) + match.captured(3));
    } else text.setText(caption);
    image.setToolTip(caption);
}

void RecordItem::load_thumbmnail(const QString& picture) {
    auto pic = QImage(picture);
    if (pic.isNull()) pic = QImage(picture.chopped(3) + "jpg");
    image.setPixmap(QPixmap::fromImage(pic.scaled(QSize(160, 90), Qt::KeepAspectRatio)));
}

VK_Manager* RecordFrame::manager;
QVector<Record>* RecordPreview::records;
QMap<int, int>* RecordPreview::logs;
QList<RecordPreview*>* RecordPreview::selected_records;

RecordFrame::RecordFrame(const QString& link, qreal k) {
    auto response = manager->get_url(link);
    connect(response, &QNetworkReply::finished, [this, response, k](){
        response->deleteLater();
        if (response->error() != QNetworkReply::NoError) return;
        QImageReader reader(response);
        QImage loaded_image = reader.read();
        setPixmap(QPixmap::fromImage(loaded_image.scaled(QSize(400, 220)/k, Qt::KeepAspectRatio)));
    });
}

RecordPreview::RecordPreview(const Record& record, int index, const QDateTime& time) :
    RecordBase(record, index), time(time)
{
    log_info.setFont(text.font());
    update_images(record.links);
    update_log_info(record.ids.first());
    layout.addWidget(&number,0,0);
    layout.addLayout(&images_layout,0,1);
    layout.addWidget(&text,0,2);
    layout.addWidget(&log_info,1,2);
    layout.addWidget(reroll_button,0,3);
    layout.addWidget(number_button,0,4);
    layout.addWidget(search_button,0,5);
    layout.addWidget(switch_button,0,6);
    connect(reroll_button, &QPushButton::clicked, this, &RecordPreview::reroll);
    connect(number_button, &QPushButton::clicked, this, &RecordPreview::input_number);
    connect(search_button, &QPushButton::clicked, this, &RecordPreview::search);
    connect(switch_button, &QPushButton::clicked, this, &RecordPreview::switch_with_next);
    reroll_button->setIconSize(QSize(30,30));
    number_button->setIconSize(QSize(30,30));
    search_button->setIconSize(QSize(30,30));
    switch_button->setIconSize(QSize(30,30));
}

void RecordPreview::reroll() {
    emit reroll_request(selected_records->indexOf(this));
//    set_index(QRandomGenerator::global()->bounded(RecordPreview::records->size()));
}

void RecordPreview::input_number() {
    int max = RecordPreview::records->size();
    bool ok;
    int random_index = QInputDialog::getInt(this, tr("Номер записи"),
                                 tr("Введите номер записи от 1 до %1").arg(max), index+1, 1, max, 1, &ok) - 1;
    if (!ok) return;
    set_index(random_index);
}

void RecordPreview::switch_with_next() {
    int pos = selected_records->indexOf(this);
    if (pos + 1 == selected_records->size()) return;
    RecordPreview* next = selected_records->operator[](pos+1);
    int this_index = index;
    int next_index = next->get_index();
    next->set_index(this_index);
    set_index(next_index);
}

void RecordPreview::search() {
    emit search_start(selected_records->indexOf(this));
}

void RecordPreview::clear() {
    while (images_layout.takeAt(0) != nullptr) {
        // Clearing items from the grid
    }
    for (auto frame : images) {
        delete frame;
    }
    images.clear();
}

void RecordPreview::set_index(int i) {
    clear();
    index = i;
    number.setText(QString().setNum(index + 1));
    auto record = RecordPreview::records->at(index);
    update_text(record.quote);
    update_images(record.links);
    update_log_info(record.ids.first());
}

void RecordPreview::update_log_info(int id) {
    auto font = log_info.font();
    if (logs->contains(id)) {
        QDateTime last = QDateTime::fromSecsSinceEpoch(logs->value(id), Qt::LocalTime);
        log_info.setText(QString("Публиковалось %1 дней назад").arg(last.daysTo(time)));
        font.setBold(true);
        font.setItalic(false);
    } else {
        log_info.setText(QString("Раньше не публиковалось"));
        font.setBold(false);
        font.setItalic(true);
    }
    log_info.setFont(font);
}

void RecordPreview::update_images(const QStringList& links) {
    qreal k = links.size() == 1 ? 1 : links.size() == 2 || links.size() == 4 ? 1.2 : 1.5;
    int grid_size = links.size() == 4 ? 2 : 3;
    for (int i = 0; i < links.size(); ++i) {
        images.push_back(new RecordFrame(links[i], k));
        images_layout.addWidget(images.back(),i/grid_size,i%grid_size);
    }
}

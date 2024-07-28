#include "include\record.h"

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

RecordBase::~RecordBase() {
    disconnect(this, nullptr, nullptr, nullptr);
}

void RecordBase::load_thumbmnail() {
    auto pic = QImage(path);
    if (pic.isNull()) pic = QImage(path.chopped(3) + "jpg");
    image.setPixmap(QPixmap::fromImage(pic.scaled(pic_size, Qt::KeepAspectRatio)));
}

RecordItem::RecordItem(const Record& record, int index, const QString& dir) :
    RecordBase(record, index)
{
    path = dir + record.pics[0];
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
    int days = last.daysTo(QDateTime::currentDateTime());
    text.setText(text.text() + QString("\n\t\tПубликовалось %1 %2 назад").arg(days).arg(inflect(days, "дней")));
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

RecordTitleItem::RecordTitleItem(const QString& title, const QString& pic_path, int size, int i) :
    RecordBase(), size(size)
{
    path = pic_path;
    pic_size = QSize(192, 108);
    index = i;
    for (int i = index; i < index + size; ++i) {
        title_indices.insert(i);
    }
    auto font = text.font();
    font.setPointSize(10);
    text.setFont(font);
    text.setText(title);
    box.setText("Записей: " + QString().setNum(size));
    box.setMinimumHeight(15);
    box.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    box.setChecked(true);
    layout.addWidget(&text,0,0);
    layout.addWidget(&image,1,0);
    layout.addWidget(&box,2,0);
    text.show();
}

void RecordTitleItem::set_checked(bool enable) {
    box.setChecked(enable);
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

TimeInputDialog::TimeInputDialog(const QTime& initial_time, QWidget *parent = nullptr) : QDialog(parent) {
    setWindowTitle("Редактирование времени");
    time_edit = new QTimeEdit(initial_time);
    time_edit->setDisplayFormat("HH:mm");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    label.setText("Выберите время:");
    layout.addWidget(&label);
    layout.addWidget(time_edit);
    layout.addWidget(buttons);
    setLayout(&layout);
}

TimeInputDialog::~TimeInputDialog() {
    delete buttons;
    delete time_edit;
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
    layout.addWidget(time_button,0,3);
    layout.addWidget(reroll_button,0,4);
    layout.addWidget(number_button,0,5);
    layout.addWidget(search_button,0,6);
    layout.addWidget(switch_button,0,7);
    connect(time_button, &QPushButton::clicked, this, &RecordPreview::set_time);
    connect(reroll_button, &QPushButton::clicked, this, &RecordPreview::reroll);
    connect(number_button, &QPushButton::clicked, this, &RecordPreview::input_number);
    connect(search_button, &QPushButton::clicked, this, &RecordPreview::search);
    connect(switch_button, &QPushButton::clicked, this, &RecordPreview::switch_with_next);
    time_button->setToolTip(QString("Время публикации: %1").arg(time.time().toString("hh:mm")));
    reroll_button->setToolTip("Случайный выбор");
    number_button->setToolTip("Выбор по номеру");
    search_button->setToolTip("Поиск по списку");
    switch_button->setToolTip("Сдвинуть вниз");
    time_button->setIconSize(QSize(30,30));
    reroll_button->setIconSize(QSize(30,30));
    number_button->setIconSize(QSize(30,30));
    search_button->setIconSize(QSize(30,30));
    switch_button->setIconSize(QSize(30,30));
}

RecordPreview::~RecordPreview() {
    disconnect(this, nullptr, nullptr, nullptr);
    disconnect(time_button, nullptr, nullptr, nullptr);
    disconnect(reroll_button, nullptr, nullptr, nullptr);
    disconnect(number_button, nullptr, nullptr, nullptr);
    disconnect(search_button, nullptr, nullptr, nullptr);
    disconnect(switch_button, nullptr, nullptr, nullptr);
    delete reroll_button;
    delete number_button;
    delete search_button;
    delete switch_button;
    delete time_button;
}

void RecordPreview::reroll() {
    emit reroll_request(this);
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

void RecordPreview::set_time() {
    TimeInputDialog dialog(time.time());
    if (dialog.exec() == QDialog::Accepted) {
        time.setTime(dialog.selectedTime());
        time_button->setToolTip(QString("Время публикации: %1").arg(time.time().toString("hh:mm")));
    }
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
        int days = last.daysTo(time);
        log_info.setText(QString("Публиковалось %1 %2 назад").arg(days).arg(inflect(days, "дней")));
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

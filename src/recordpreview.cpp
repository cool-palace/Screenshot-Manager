#include "include\recordpreview.h"

QVector<Record>* RecordPreview::records;
QMap<int, int>* RecordPreview::logs;
QList<RecordPreview*>* RecordPreview::selected_records;
VK_Manager* RecordFrame::manager;

RecordFrame::RecordFrame(const QString& link) {
    auto response = manager->get_url(link);
    connect(response, &QNetworkReply::finished, [this, response](){
        response->deleteLater();
        if (response->error() != QNetworkReply::NoError) return;
        QImageReader reader(response);
        QImage loaded_image = reader.read();
        setPixmap(QPixmap::fromImage(loaded_image.scaled(QSize(400, 220), Qt::KeepAspectRatio)));
        disconnect(response, nullptr, this, nullptr);
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

RecordPreview::RecordPreview(const Record& record, int index, const QDateTime& time, bool smart_tags)
    : QWidget()
    , Ui::RecordPreview()
    , index(index)
    , time(time)
{
    setupUi(this);
    update_text(record.quote);
    number->setText(QString().setNum(index + 1));
    update_images(record.links);
    update_log_info(record.ids.first());
    connect(time_button, &QPushButton::clicked, this, &RecordPreview::set_time);
    connect(search_button, &QPushButton::clicked, this, &RecordPreview::search);
    connect(switch_button, &QPushButton::clicked, this, &RecordPreview::switch_with_next);
    time_button->setToolTip(QString("Время публикации: %1").arg(time.time().toString("hh:mm")));
    spinbox->hide();
}

RecordPreview::RecordPreview(const Record& record, int index, const QDateTime& time) :
    RecordPreview(record, index, time, false)
{
    connect(reroll_button, &QPushButton::clicked, this, &RecordPreview::reroll);
    connect(number_button, &QPushButton::clicked, this, &RecordPreview::input_number);
}

RecordPreview::RecordPreview(const Record& record, const QDateTime& time, const QStringList& tags, const QList<int>& record_set) :
    RecordPreview(record, record_set.first(), time, false)
{
    hashtags = tags;
    record_variants = record_set;
    int size = record_variants.size();
    spinbox->setMaximum(size);
    spinbox->setSuffix(QString("/%1").arg(size));
    spinbox->show();
    connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecordPreview::spinbox_changed);
    reroll_button->hide();
    number_button->hide();
}

RecordPreview::~RecordPreview() {
    for (auto image : images) {
        delete image;
    }
    disconnect(this, nullptr, nullptr, nullptr);
    disconnect(time_button, nullptr, nullptr, nullptr);
    disconnect(reroll_button, nullptr, nullptr, nullptr);
    disconnect(number_button, nullptr, nullptr, nullptr);
    disconnect(search_button, nullptr, nullptr, nullptr);
    disconnect(switch_button, nullptr, nullptr, nullptr);
    disconnect(spinbox, nullptr, nullptr, nullptr);
    delete reroll_button;
    delete number_button;
    delete search_button;
    delete switch_button;
    delete time_button;
    delete spinbox;
}

void RecordPreview::update_text(const QString& caption) {
    QRegularExpression regex("(.*?)?([#&])(.*)?$");
    auto i = regex.globalMatch(caption);
    if (i.hasNext()) {
        auto match = i.peekNext();
        text->setText(match.captured(1) + '\n' + match.captured(2) + match.captured(3));
    } else text->setText(caption);
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
    auto this_tag_pair = get_tags();
    auto next_tag_pair = next->get_tags();
    next->set_tags(this_tag_pair.first, this_tag_pair.second);
    set_tags(next_tag_pair.first, next_tag_pair.second);
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
    while (images_layout->takeAt(0) != nullptr) {
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
    number->setText(QString().setNum(index + 1));
    auto record = RecordPreview::records->at(index);
    update_text(record.quote);
    update_images(record.links);
    update_log_info(record.ids.first());
}

void RecordPreview::set_tags(const QStringList& tags, const QList<int>& records) {
    hashtags = tags;
    record_variants = records;
    int size = records.size();
    disconnect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecordPreview::spinbox_changed);
    spinbox->setMaximum(size);
    spinbox->setValue(record_variants.indexOf(index)+1);
    spinbox->setSuffix(QString("/%1").arg(size));
    connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecordPreview::spinbox_changed);
}

void RecordPreview::update_log_info(int id) {
    auto font = log_info->font();
    if (logs->contains(id)) {
        QDateTime last = QDateTime::fromSecsSinceEpoch(logs->value(id), Qt::LocalTime);
        int days = last.daysTo(time);
        log_info->setText(QString("Публиковалось %1 %2 назад").arg(days).arg(inflect(days, "дней")));
        font.setBold(true);
        font.setItalic(false);
    } else {
        log_info->setText(QString("Раньше не публиковалось"));
        font.setBold(false);
        font.setItalic(true);
    }
    log_info->setFont(font);
}

void RecordPreview::update_images(const QStringList& links) {
    for (int i = 0; i < links.size(); ++i) {
        images.push_back(new RecordFrame(links[i]));
        images_layout->addWidget(images.back());
    }
    images_layout->setAlignment(links.size() > 1 ? Qt::AlignLeft : Qt::AlignHCenter);
}

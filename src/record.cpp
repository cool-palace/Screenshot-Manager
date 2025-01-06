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

RecordBase::RecordBase(const Record& record, int index) : RecordBase(record.quote, index) { }

RecordBase::RecordBase(const QString& quote, int index) : QWidget(), index(index) {
    update_text(quote);
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
    RecordTitleItem(title, pic_path, size)
{
    index = i;
    for (int i = index; i < index + size; ++i) {
        title_indices.insert(i);
    }
}

RecordTitleItem::RecordTitleItem(const QString& title, const QString& pic_path, int size) :
        RecordBase(), size(size)
{
    path = pic_path;
    pic_size = QSize(192, 108);
    image.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto font = text.font();
    font.setPointSize(10);
    text.setFont(font);
    text.setText(title);
    text.setMaximumWidth(pic_size.width());
    text.setMaximumHeight(50);
    text.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
    box.setText("Записей: " + QString().setNum(size));
    box.setMinimumHeight(20);
    box.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    box.setChecked(true);
    layout.addWidget(&text,0,0);
    layout.addWidget(&image,1,0);
    layout.addWidget(&box,2,0);
    text.show();
}

void RecordTitleItem::set_checked(bool enable) {
    box.setChecked(enable);
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

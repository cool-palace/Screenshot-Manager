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

RecordItem::RecordItem(const Record& record, int index, const QString& path) :
    QWidget(),
    index(index)
{
    QtConcurrent::run(this, &RecordItem::load_thumbmnail, path + record.pics[0]);
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
    box.setText("Кадров: " + QString().setNum(record.pics.size()));
    box.setMinimumHeight(15);
    box.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    box.setChecked(record.is_public);
    box.setEnabled(false);
    box.hide();
    layout.setContentsMargins(0,0,0,0);
    layout.addWidget(&number,0,0);
    layout.addWidget(&image,0,1);
    layout.addWidget(&text,0,2);
    layout.addWidget(&box,1,1);
    setLayout(&layout);
}

RecordItem::RecordItem(const QString& quote, int index) :
    QWidget(),
    index(index)
{
    text.setText(quote);
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
    box.setMinimumHeight(15);
    box.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    box.setEnabled(false);
    box.hide();
    layout.setContentsMargins(0,0,0,0);
    layout.addWidget(&number,0,0);
    layout.addWidget(&image,0,0);
    layout.addWidget(&text,0,1);
    layout.addWidget(&box,1,0);
    setLayout(&layout);
}

void RecordItem::mouseDoubleClickEvent(QMouseEvent* e) {
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

void RecordItem::update_text(const QString& caption) {
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

RecordPreview::RecordPreview(const Record& record, int index) :
    RecordItem(record, index, ""),
    record(record)
{
//    set_list_view();
}

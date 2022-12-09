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
    index(index),
    image(new QLabel),
    text(new QLabel(record.quote)),
    box(new QCheckBox("Кадров: " + QString().setNum(record.pics.size()))),
    layout(new QGridLayout)
{
//    auto pic = QImage(path + record.pics[0]);
    auto pic = QImage(path + record.pics[0].chopped(3) + "jpg");
    image->setPixmap(QPixmap::fromImage(pic.scaled(QSize(160, 120), Qt::KeepAspectRatio)));
    image->setToolTip(record.quote);
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    text->setMaximumHeight(120);
    text->setWordWrap(true);
    auto font = text->font();
    font.setPointSize(12);
    text->setFont(font);
    text->hide();
    box->setMinimumHeight(15);
    box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    box->setChecked(record.is_public);
    box->setEnabled(false);
    box->hide();
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(image,0,0);
    layout->addWidget(text,0,1);
    layout->addWidget(box,1,0);
    setLayout(layout);
}

RecordItem::~RecordItem() {
    delete image;
    delete text;
    delete box;
    delete layout;
}

void RecordItem::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton ) {
        emit selected(index);
    }
}

void RecordItem::set_gallery_view() {
    text->hide();
    box->show();
    show();
}
void RecordItem::set_list_view() {
    box->hide();
    text->show();
    show();
}

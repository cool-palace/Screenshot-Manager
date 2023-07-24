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

RecordFrame::RecordFrame(const QString& link) {
    auto response = manager->get_url(link);
    connect(response, &QNetworkReply::finished, [this, response](){
        response->deleteLater();
        if (response->error() != QNetworkReply::NoError) return;
        QImageReader reader(response);
        QImage loaded_image = reader.read();
        setPixmap(QPixmap::fromImage(loaded_image.scaled(QSize(400, 220), Qt::KeepAspectRatio)));
    });
}

RecordPreview::RecordPreview(const Record& record, int index) :
    RecordBase(record, index)
{
    layout.addWidget(&number,0,0);
    for (int i = 0; i < record.links.size(); ++i) {
        images.push_back(new RecordFrame(record.links[i]));
        layout.addWidget(images.back(),0,i+1);
    }
    layout.addWidget(&text,0,record.links.size() + 1);
    layout.addWidget(reroll_button,0,record.links.size() + 2);
    layout.addWidget(number_button,0,record.links.size() + 3);
    connect(reroll_button, &QPushButton::clicked, this, &RecordPreview::reroll);
    connect(number_button, &QPushButton::clicked, this, &RecordPreview::input_number);
}

void RecordPreview::reroll() {
    clear();
    index = QRandomGenerator::global()->bounded(RecordPreview::records->size());
    set_index(index);
}

void RecordPreview::input_number() {
    int max = RecordPreview::records->size();
    bool ok;
    index = QInputDialog::getInt(this, tr("Ручной ввод номера записи"),
                                 tr("Введите номер записи от 1 до %1").arg(max), index+1, 1, max, 1, &ok) - 1;
    if (!ok) return;
    clear();
    set_index(index);
}

void RecordPreview::clear() {
    while (layout.takeAt(0) != nullptr) {
        // Clearing items from the grid
    }
    for (auto frame : images) {
        delete frame;
    }
    images.clear();
}

void RecordPreview::set_index(int index) {
    number.setText(QString().setNum(index + 1));
    layout.addWidget(&number,0,0);
    auto record = RecordPreview::records->at(index);
    update_text(record.quote);
    for (int i = 0; i < record.links.size(); ++i) {
        images.push_back(new RecordFrame(record.links[i]));
        layout.addWidget(images.back(),0,i+1);
    }
    layout.addWidget(&text,0,record.links.size() + 1);
    layout.addWidget(reroll_button,0,record.links.size() + 2);
    layout.addWidget(number_button,0,record.links.size() + 3);
}

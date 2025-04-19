#include "text_labeling.h"
#include <QFileDialog>
#include <QPainter>
#include <QDebug>
#include <QKeyEvent>
#include <QJsonArray>
#include <QMessageBox>
#define SCREENSHOTS 0
#define LABELS 1



TextLabeling::TextLabeling(QWidget *parent) : QWidget(parent) {
    setupUi(this);

    locations[SCREENSHOTS] = "C:\\Users\\User\\Pictures\\Light Alloy\\";
    locations[LABELS] = "C:\\Users\\User\\Documents\\Screenshot-Manager\\labels\\";
    connect(slider, &QAbstractSlider::valueChanged, this, &TextLabeling::slider_change);
    connect(pbBack, &QPushButton::clicked, this, &TextLabeling::back_button);
    connect(pbOk, &QPushButton::clicked, this, &TextLabeling::ok_button);
    connect(pbSkip, &QPushButton::clicked, this, &TextLabeling::skip_button);
    connect(pbRemoveLine, &QPushButton::clicked, this, &TextLabeling::remove_line_button);
    connect(pbEditStlye, &QPushButton::clicked, this, &TextLabeling::edit_style_button);
    connect(chbX, &QCheckBox::clicked, this, &TextLabeling::enable_x);
    connect(dsbX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextLabeling::box_change);
    connect(dsbY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextLabeling::box_change);
    connect(dsbHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextLabeling::box_change);

    connect(pbAddLine, &QPushButton::clicked, this, &TextLabeling::add_line);
    connect(pbAddStyle, SIGNAL(clicked(bool)), this, SLOT(add_style(bool)));
    connect(pbSave, &QPushButton::clicked, this, &TextLabeling::save_labels);
    connect(cbLines, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TextLabeling::update_style);

    connect(rbStyle, &QRadioButton::toggled, this, &TextLabeling::switch_mode);
//    connect(parent, &MainWindow::key_press, this, &TextLabeling::keyPressEvent);
//    connect(parent, &MainWindow::key_release, this, &TextLabeling::keyReleaseEvent);


}

QPixmap TextLabeling::scaled_with_box(QImage source) const {
    QPainter painter(&source);

    // Вычисление размеров
    int imgWidth = source.width();
    int imgHeight = source.height();

    // Проверяем существование уже заданной разметки на этот кадр
    if (m_labels.size() > pic_index || (m_labels.size() == pic_index && m_labels.at(pic_index).size() > 0)) {
        // Рисуем обнаруженную разметку зелёным
        const QList<XLabel> labels = m_labels[pic_index];
        for (int i = 0; i < labels.size(); ++i) {
            int index = labels[i].i;
            if (index < 0) continue;

            double x = labels[i].x;
            float w = 1.0f - 2 * x;

            QRectF bbox(x * imgWidth, m_styles[index].y * imgHeight, w * imgWidth, m_styles[index].h * imgHeight);

            QPen pen(Qt::green, 3);
            painter.setPen(pen);
            painter.drawRect(bbox);
        }
    }
    if (chbX->isChecked()) {
        // Новую разметку рисуем красным
        double x, y, h;
        if (rbStyle->isChecked()) {
            x = 0;
            y = dsbY->value();
            h = dsbHeight->value();
        } else {
            LineStyle line = m_styles[cbLines->currentIndex()];
            x = dsbX->value();
            y = line.y;
            h = line.h;
        }

        float w = 1.0f - 2 * x;

        QRectF bbox(x * imgWidth, y * imgHeight, w * imgWidth, h * imgHeight);

        QPen pen(Qt::red, 3, Qt::DashDotDotLine);
        painter.setPen(pen);
        painter.drawRect(bbox);
    }
    return QPixmap::fromImage(source.scaled(image->geometry().size(), Qt::KeepAspectRatio));
}

QString TextLabeling::title_name(int index) {
    return title;
}

void TextLabeling::start() {
    QDir dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                    locations[SCREENSHOTS]));
    if (dir.isEmpty()) return;

    clear_all();

    pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    m_labels.reserve(pics.size());
    for (int i = 0; i < pics.size(); ++i) {
        m_labels.append(QList<XLabel>());
    }
    slider->setMaximum(pics.size() - 1);
    slider->blockSignals(false);
    rbLines->blockSignals(false);
    rbStyle->blockSignals(false);
    title = dir.dirName();

    QString filepath = locations[LABELS] + title + ".json";
    QFile file(filepath);
    if (file.exists()) {
        load_labels(json_object(filepath));
    }
    set_enabled(true);
    draw(pic_index = 0);
}

void TextLabeling::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_F1:
        pbOk->click();
        break;
    case Qt::Key_F2:
        pbAddLine->click();
        break;
    default:
        break;
    }
}

void TextLabeling::keyReleaseEvent(QKeyEvent *event) {

}

void TextLabeling::slider_change(int value) {
    pic_index = value;
    draw(pic_index);
    show_status();
}

void TextLabeling::box_change() {
    draw(pic_index);
}

void TextLabeling::style_change(int index) {
    LineStyle line = m_styles[index];
    dsbY->setValue(line.y);
    dsbHeight->setValue(line.h);
    draw(pic_index);
}

void TextLabeling::back_button() {
    if (pic_index == 0) return;
    draw(--pic_index);
    slider->setValue(pic_index);
}

void TextLabeling::ok_button() {
    if (!cbLines->count())
        return;
    if (pic_index >= pics.size())
        return;

    if (chbX->isChecked()) {
        // Добавляем новую разметку для кадра
        int style_index = cbLines->currentIndex();
        XLabel current_label = XLabel(dsbX->value(), style_index);
        m_labels[pic_index].append(current_label);
    }
    int current_size = m_labels[pic_index].size();
    int current_style = cbLines->currentIndex();
    // Если на предыдущем кадре было несколько строк, то удобно при переходе к новому кадру найти стиль нижней строки
    if (current_size > 1 && current_style >= current_size - 1) {
        cbLines->setCurrentIndex(current_style - current_size + 1);
    }
    if (pic_index + 1 < pics.size()) {
        slider->setValue(++pic_index);
    } else {
//        set_enabled(false);
        save_labels();
    }
}

void TextLabeling::skip_button() {
    if (pic_index < pics.size()) {
        draw(++pic_index);
        slider->setValue(pic_index);
    } else {
        save_labels();
//        set_enabled(false);
//        save_title_journal(title_name());
    }
}

void TextLabeling::remove_line_button() {
    QList<XLabel>& labels = m_labels[pic_index];
    if (labels.size()) {
        labels.pop_back();
        box_change();
    }
}

void TextLabeling::edit_style_button() {
    m_styles[cbLines->currentIndex()] = LineStyle(dsbY->value(), dsbHeight->value());
    box_change();
}

void TextLabeling::add_line() {
    int style_index = cbLines->currentIndex();
    for (const XLabel& label : m_labels[pic_index]) {
        // Если строка с заданным стилем уже существует для данного кадра, то заново её не добавляем
        if (label.i == style_index)
            return;
    }
    XLabel current_label = XLabel(dsbX->value(), style_index);
    m_labels[pic_index].append(current_label);

    if (cbLines->count() > style_index + 1) {
        cbLines->setCurrentIndex(style_index + 1);
    } else rbStyle->setChecked(true);
}

void TextLabeling::add_style(bool b) {
    add_style(dsbY->value(), dsbHeight->value());
    cbLines->setCurrentIndex(cbLines->count() - 1);
}

void TextLabeling::add_style(double y, double h) {
    m_styles.append(LineStyle(y, h));
    cbLines->blockSignals(true);
    cbLines->addItem(QString("Ряд %1").arg(cbLines->count() + 1));
    cbLines->blockSignals(false);

    if (!rbLines->isEnabled())
        rbLines->setEnabled(true);
}

void TextLabeling::switch_mode() {
    update_style(cbLines->currentIndex());
    set_enabled(true);
}

void TextLabeling::draw(int index = 0) {
    auto dir_path = locations[SCREENSHOTS] + title_name() + QDir::separator();
    auto pic = QImage(dir_path + pics[index]);
    image->setPixmap(scaled_with_box(pic));
    bool reached_end = index + 1 >= pics.size();
    pbBack->setEnabled(index > 0);
    pbRemoveLine->setEnabled(m_labels[pic_index].size());
}

void TextLabeling::show_status() {
    //
}

bool TextLabeling::data_ready() {
    return true;
}

void TextLabeling::enable_x(bool checked) {
    dsbX->setEnabled(checked);
    box_change();
}

void TextLabeling::update_style(int index) {
    const LineStyle& line = m_styles[index];
    dsbY->setValue(line.y);
    dsbHeight->setValue(line.h);
    box_change();
}

void TextLabeling::set_enabled(bool enable) {
    bool style_mode = rbStyle->isChecked();
    dsbY->setEnabled(enable && style_mode);
    dsbHeight->setEnabled(enable && style_mode);
    pbAddStyle->setEnabled(enable && style_mode);
    pbEditStlye->setEnabled(enable && style_mode && m_styles.size());

    cbLines->setEnabled(enable && !style_mode);
    pbOk->setEnabled(enable && !style_mode);
    dsbX->setEnabled(enable && !style_mode);
    pbAddLine->setEnabled(enable && !style_mode);
}

void TextLabeling::load_labels(const QJsonObject& json_file) {
    title = json_file.value("title").toString();

    auto style_array = json_file.value("styles").toArray();
    for (QJsonValueRef s : style_array) {
        auto object = s.toObject();
        add_style(object["y"].toDouble(), object["h"].toDouble());
    }
    auto pic_array = json_file.value("pics").toArray();
    for (QJsonValueRef p : pic_array) {
        auto object = p.toObject();
        int index = pics.indexOf(object["filename"].toString());
        auto label_array = object["labels"].toArray();
        for (QJsonValueRef l : label_array) {
            auto object = l.toObject();
            m_labels[index].append(XLabel(object["x"].toDouble(), object["style"].toInt()));
        }
    }
    dsbY->blockSignals(true);
    dsbHeight->blockSignals(true);
    dsbY->setValue(m_styles.first().y);
    dsbHeight->setValue(m_styles.first().h);
    dsbY->blockSignals(false);
    dsbHeight->blockSignals(false);
}

void TextLabeling::save_labels() {
    QJsonArray style_array;
    for (const auto& style: m_styles) {
        style_array.push_back(style.to_json());
    }

    QJsonArray pics_array;
    for (int i = 0; i < pics.size(); ++i) {
        QJsonObject object;
        QJsonArray label_array;
        for (const auto& label: m_labels[i]) {
            label_array.push_back(label.to_json());
        }
        object["filename"] = pics[i];
        object["labels"] = label_array;
        pics_array.append(object);
    }

    QJsonObject object;
    object["title"] = title_name();
    object["styles"] = style_array;
    object["pics"] = pics_array;

    QFile file(locations[LABELS] + title_name() + ".json");
    auto message = save_json(object, file)
            ? "Файл с разметкой сохранён."
            : QString("Не удалось сохранить файл: %1").arg(file.fileName());
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Information);
    QPushButton* openFolderBtn = msgBox.addButton("Открыть другую папку", QMessageBox::ActionRole);
    QPushButton* continueBtn = msgBox.addButton("Продолжить работу", QMessageBox::AcceptRole);
    msgBox.exec();
    // Проверяем, что нажал пользователь
    if (msgBox.clickedButton() == openFolderBtn) {
        start();
    }
}

void TextLabeling::clear_all() {
    pics.clear();
    title.clear();
    m_styles.clear();
    m_labels.clear();
    cbLines->blockSignals(true);
    cbLines->clear();
    slider->blockSignals(true);
    slider->setValue(pic_index = 0);
    rbLines->blockSignals(true);
    rbLines->setDisabled(true);
    rbStyle->blockSignals(true);
    rbStyle->setChecked(true);
}

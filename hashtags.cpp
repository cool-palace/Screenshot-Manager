#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

void MainWindow::get_hashtags() {
    QFile file("hashtags.txt");
    if (!file.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage("Не удалось открыть файл с хэштегами.");
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString tag = in.readLine();
        if (tag[0] == '#') {
            tag.remove(0,1);
        }
        create_hashtag_button(tag);
    }
    file.close();
    update_hashtag_grid();
}

void MainWindow::create_hashtag_button(const QString& text) {
    hashtags.insert(text, new QPushButton(text));
    hashtags[text]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    hashtags[text]->setToolTip(text);
    connect(hashtags[text], &QPushButton::clicked, [this, text]() {
        if (current_mode == IDLE) return;
        QRegularExpression regex("(.*)?#" + text + "(\\s.*)?$");
        auto i = regex.globalMatch(ui->text->toPlainText());
        bool hashtag_is_in_text = i.hasNext();
        if (!hashtag_is_in_text) {
            ui->text->setText(ui->text->toPlainText() + " #" + text);
        } else {
            auto match = i.peekNext();
            ui->text->setText(match.captured(1).chopped(1) + match.captured(2));
        }
        highlight_button(hashtags[text], !hashtag_is_in_text);
    });
}

void MainWindow::update_hashtag_grid() {
    while (ui->tag_grid->takeAt(0) != nullptr) {
        // Clearing buttons from the grid
    }
    int i = 0;
    for (auto button : hashtags) {
        ui->tag_grid->addWidget(button, i / 10, i % 10);
        ++i;
    }
    ui->tag_grid->addWidget(add_tag_button, i / 10, i % 10);
}

void MainWindow::load_hashtag_info() {
    for (auto record : records) {
        auto i = hashtag_match(record.quote);
        while (i.hasNext()) {
            auto match = i.next().captured(1);
            if (hashtags_count.contains(match)) {
                ++hashtags_count[match];
            } else {
                hashtags_count[match] = 1;
            }
        }
    }
    for (auto hashtag : hashtags_count.keys()) {
        auto button = hashtags.value(hashtag);
        if (button) {
            button->setText(button->toolTip() + ' ' + QString().setNum(hashtags_count[hashtag]));
            button->setFlat(true);
        }
    }
}

void MainWindow::recalculate_hashtags(bool increase) {
    for (const auto& hashtag : current_hashtags) {
        size_t count = increase ? ++hashtags_count[hashtag] : --hashtags_count[hashtag];
        auto button = hashtags.value(hashtag);
        if (button) {
            button->setText(button->toolTip() + ' ' + QString().setNum(count));
            button->setFlat(count);
        }
    }
}

void MainWindow::update_hashtag_info() {
    recalculate_hashtags(false);
    update_current_hashtags();
    recalculate_hashtags(true);
}

void MainWindow::update_current_hashtags() {
    highlight_current_hashtags(false);
    current_hashtags.clear();
    auto i = hashtag_match(records[pic_index].quote);
    while (i.hasNext()) {
        current_hashtags.push_back(i.next().captured(1));
    }
    highlight_current_hashtags(true);
}

QRegularExpressionMatchIterator MainWindow::hashtag_match(const QString& text) {
    QRegularExpression regex("#([а-яё0-9_]+)");
    return regex.globalMatch(text);
}

void MainWindow::highlight_current_hashtags(bool enable) {
    for (auto hashtag : current_hashtags) {
        auto button = hashtags.value(hashtag);
        if (!button) {
            qDebug() << "Unexpected tag: " << hashtag;
            return;
        }
        highlight_button(button, enable);
    }
}

void MainWindow::highlight_button(QPushButton * button, bool enable) {
    auto font = button->font();
    font.setBold(enable);
    button->setFont(font);
}

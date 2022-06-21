#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->open_folder, &QAction::triggered, [this]() {
        if (ui->ok->isEnabled()) {
            quotes.clear();
        }
        pics.clear();
        dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами", "E:/Compressed/"));
        pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        show_status();
        set_enabled(!quotes.empty());
        if (!pics.empty()) {
            draw(0);
        }
    });

    connect(ui->open_doc, &QAction::triggered, [this]() {
        if (ui->ok->isEnabled()) {
            pics.clear();
        }
        quotes.clear();
        auto filepath = QFileDialog::getOpenFileName(nullptr, "Открыть файл с цитатами",
                                                     "C:\\Users\\User\\PycharmProjects\\Quote_Parser\\docs",
                                                     "Текстовые документы (*.txt)");
        QFile file(filepath);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            in.setCodec("UTF-8");
            while (!in.atEnd()) {
                quotes.push_back(in.readLine());
            }
            file.close();
        } else {
            ui->statusBar->showMessage("Не удалось открыть файл.");
            return;
        }
        show_status();
        if (!quotes.empty()) {
            show_text(0);
        }
        set_enabled(!pics.empty());
    });

    connect(ui->add, &QPushButton::clicked, [this]() {
        pic_end_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_end_index);
    });

    connect(ui->ok, &QPushButton::clicked, [this]() {
        pic_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_index);
        show_text(++quote_index);
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::set_enabled(bool enable) {
    ui->back->setEnabled(enable);
    ui->ok->setEnabled(enable);
    ui->skip->setEnabled(enable);
    ui->add->setEnabled(enable);
    ui->text->setEnabled(enable);
    ui->text_back->setEnabled(enable);

    if (!enable) return;
    quote_index = pic_index = pic_end_index = 0;
}

QPixmap MainWindow::scaled(const QImage& source) {
    return QPixmap::fromImage(source.scaled(ui->image->geometry().size(), Qt::KeepAspectRatio));
}

void MainWindow::draw(int index) {
    if (pics.size() <= index) return;
    auto image = QImage(dir.path() + QDir::separator() + pics[index]);
    ui->image->setPixmap(scaled(image));
}

void MainWindow::show_text(int index) {
    if (quotes.size() <= index) return;
    ui->text->setText(quotes[index]);
}

void MainWindow::show_status() {
    if (!pics.empty() && quotes.empty()) {
        ui->statusBar->showMessage("Загружено " + QString().setNum(pics.size()) + " кадров. Откройте документ с цитатами.");
    } else if (pics.empty() && !quotes.empty()) {
        ui->statusBar->showMessage("Загружено " + QString().setNum(quotes.size()) + " цитат. Откройте папку с кадрами.");
    } else {
        ui->statusBar->showMessage("Осталось кадров: " + QString().setNum(pics.size()) + ", цитат: " + QString().setNum(quotes.size()) + ".");
    }
}

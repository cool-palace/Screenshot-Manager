#include "include\common.h"
#include <QtMath>

CaptchaDialog::CaptchaDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Введите капчу");
    setMinimumSize(QSize(300,200));
    setModal(true);
    captcha.setScaledContents(true);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    captcha.setPixmap(QPixmap::fromImage(QImage(":/images/icons8-full-image-80.png")));
    captcha.show();
    layout.addWidget(&captcha);
    layout.addWidget(&line_edit);
    layout.addWidget(buttons);
    setLayout(&layout);
    hide();
}

CaptchaDialog::~CaptchaDialog() {
    delete buttons;
}

void CaptchaDialog::set_captcha_image(const QImage & image) {
    captcha.setPixmap(QPixmap::fromImage(image));
    line_edit.clear();
}

ProgressDialog::ProgressDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Компиляция журналов в базу данных");
    setFixedSize(QSize(500,120));
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    progress.setRange(0,100);
    layout.addWidget(&text);
    layout.addWidget(&progress);
    layout.addWidget(buttons);
    setLayout(&layout);
}

ProgressDialog::~ProgressDialog() {
    delete buttons;
}

void ProgressDialog::update_progress(int value, const QString &description) {
    progress.setValue(value);
    text.setText(description);
    if (value == 100) {
        buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        buttons->button(QDialogButtonBox::Abort)->setEnabled(false);
    }
}

QJsonObject json_object(const QString& filepath) {
    QFile config(filepath);
    if (!config.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QJsonObject();
    }
    QString s = QString::fromUtf8(config.readAll());
    config.close();
    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    QJsonObject json_file = doc.object();
    return json_file;
}

bool save_json(const QJsonObject& object, QFile& file) {
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QJsonDocument(object).toJson();
    file.close();
    return true;
}

QStringList word_forms(const QString& word) {
    if (word == "Найдено") {
        return QStringList() << "Найдена" << word << word;
    } else if (word == "записей") {
        return QStringList() << "запись" << "записи" << word;
    } else if (word == "фильтрам") {
        return QStringList() << "фильтру" << word << word;
    } else if (word == "дней") {
        return QStringList() << "день" << "дня" << word;
    } else if (word == "фотографий") {
        return QStringList() << "фотографию" << "фотографии" << word;
    } else if (word == "кадров") {
        return QStringList() << "кадр" << "кадра" << word;
    } else return QStringList() << word << word << word;
}

QString inflect(int i, const QString& word) {
    QStringList forms = word_forms(word);
    if ((i % 100 - i % 10) != 10) {
        if (i % 10 == 1) {
            return forms[0];
        } else if (i % 10 > 1 && i % 10 < 5) {
            return forms[1];
        }
    }
    return forms[2];
}

int lowest_degree_vertex(const QList<QList<int>> &M){
    int vertex = 0;
    int lowest_vertex_degree = M.size() - 1;
    for (int i = 0; i < M.size(); ++i) {
        int current_vertex_degree = 0;
        for (int j = 0; j < M[i].size(); ++j) {
            if (i != j && M[i][j] > 0) ++current_vertex_degree;
        }
        if (current_vertex_degree < lowest_vertex_degree) {
            lowest_vertex_degree = current_vertex_degree;
            vertex = i;
        }
    }
    return vertex;
}

void find_hamiltonian_cycles(int current, const QList<QList<int>> &M, QVector<int> &path, QSet<int> &visited, QList<QVector<int>> &cycles, int start) {
    // Adding cycle if the path contains all the vertices and there is an edge to the starting vertex
    if (path.size() == M.size()) {
        if (M[current][start] > 0) {
            path.append(start);
            cycles.append(path);
            path.removeLast();
        }
        return;
    }
    for (int next = 0; next < M.size(); ++next) {
        if (current != next && M[current][next] > 0 && !visited.contains(next)) {
            visited.insert(next);
            path.append(next);
            find_hamiltonian_cycles(next, M, path, visited, cycles, start);
            path.removeLast();
            visited.remove(next);
        }
    }
}

QList<QVector<int>> get_all_hamiltonian_cycles(const QList<QList<int>> &M) {
    QList<QVector<int>> cycles;
    QVector<int> path;
    QSet<int> visited;
    int start = lowest_degree_vertex(M);
    path.append(start);
    visited.insert(start);
    find_hamiltonian_cycles(start, M, path, visited, cycles, start);
    return remove_duplicate_cycles(cycles);
}

QList<QVector<int>> remove_duplicate_cycles(const QList<QVector<int>>& cycles) {
    QList<QVector<int>> unique_cycles;
    for (const auto& cycle : cycles) {
        QVector<int> reversed_cycle = cycle;
        std::reverse(reversed_cycle.begin(), reversed_cycle.end());
        bool is_duplicate = false;
        for (const auto& unique_cycle : unique_cycles) {
            if (cycle == unique_cycle || reversed_cycle == unique_cycle) {
                is_duplicate = true;
                break;
            }
        }
        if (!is_duplicate) {
            unique_cycles.append(cycle);
        }
    }
    return unique_cycles;
}

double roundTo(double value, int decimals) {
    double factor = qPow(10.0, decimals);
    return qRound(value * factor) / factor;
}

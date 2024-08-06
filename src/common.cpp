#include "include\common.h"

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

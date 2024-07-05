#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

void MainWindow::read_descriptions(const QJsonObject& json_file) {
//    auto info_json = json_object(locations[JOURNALS] + "result\\photo_info.json");
//    for (auto it = json_file.begin(); it != json_file.end(); ++it) {
//        auto info = info_json.value(it.key()).toObject();
//        Record record;
//        record.ids.append(it.key().toInt());
//        auto path = info.value("path").toString().split('\\');
////        qDebug() << path;
//        auto title = path.first();
//        if (title_map.isEmpty() || title_map.last() != title) {
////            title_map[quotes.size()] = title;
//            title_map[records.size()] = title;
//        }
//        record.pics.append(path.back());
//        record.links.append(info["link"].toString());
//        record.quote = it.value().toString();
//        record.is_public = true;
//        records.append(record);
////        photo_ids.append(it.key().toInt());
////        links.append(info["link"].toString());
////        pics.append(path.back());
////        quotes.append(it.value().toString());
//    }
////    qDebug() << records.size();
////    qDebug() << records.first().quote << records.first().pics << records.first().links;
//    qDebug() << title_map;
}

void MainWindow::compile_journals() {
    QDir dir = QDir(locations[JOURNALS]);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    if (configs.contains(".test.json")) {
        configs.removeAt(configs.indexOf(".test.json"));
    }
    QJsonArray resulting_array;
    QJsonArray hidden_array;
    QJsonObject title_map;
    for (const auto& config : configs) {
        auto object = json_object(locations[JOURNALS] + config);
        auto title = object["title"].toString();
        title_map[QString().setNum(resulting_array.size())] = title;
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
            record["index"] = (record["public"].toBool() ? resulting_array : hidden_array).size();
            (record["public"].toBool() ? resulting_array : hidden_array).push_back(record);
        }
    }
    QJsonObject result, hidden_result;
    result["records"] = resulting_array;
    result["reverse_index"] = reverse_index(resulting_array);
    result["title_map"] = title_map;
    hidden_result["records"] = hidden_array;
    hidden_result["reverse_index"] = reverse_index(hidden_array);
    QFile file(locations[JOURNALS] + "result\\public_records.json");
    QFile hidden_file(locations[JOURNALS] + "result\\hidden_records.json");
    save_json(hidden_result, hidden_file);
    auto message = save_json(result, file)
            ? "Обработано конфигов: " + QString().setNum(configs.size()) + ", "
              "скомпилировано публичных записей: " + QString().setNum(resulting_array.size()) + ", "
              "скрытых записей: " + QString().setNum(hidden_array.size())
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

void MainWindow::export_text() {
    QFile file("exported_text.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ui->statusBar->showMessage("Не удалось открыть выходной файл.");
        return;
    }
    QRegularExpression regex("(.*?)\\s[#&].*$");
    QTextStream out(&file);
    out.setCodec("UTF-8");
    QDir dir = QDir(locations[JOURNALS]);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto& config : configs) {
//        if (config.startsWith("Kono")) break;
        auto object = json_object(locations[JOURNALS] + config);
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
            if (record["public"].toBool()) {
                auto quote = record["caption"].toString();
//                auto i = regex.globalMatch(quote);
//                if (i.hasNext()) {
//                    auto match = i.next();
//                    quote = match.captured(1);
//                }
                out << quote + "\r\n";
            }
        }
    }
    file.close();
}

QJsonObject MainWindow::reverse_index(const QJsonArray& array) {
    QJsonObject result;
    for (int index = 0; index < array.size(); ++index) {
        auto id_array = array[index].toObject()["photo_ids"].toArray();
        for (QJsonValueRef id : id_array) {
            result[QString().setNum(id.toInt())] = index;
        }
    }
    return result;
}

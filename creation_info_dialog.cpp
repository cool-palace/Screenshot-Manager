#include "creation_info_dialog.h"
#include <QMessageBox>
#include <QDateTime>
#include "include/common.h"
#include "locations.h"

CreationInfoDialog::CreationInfoDialog(const QString& album_name, QWidget *parent) : QDialog(parent) {
    setupUi(this);

    setWindowTitle("Новый журнал: " + album_name);
    QString series = seriesName(album_name);
    m_seriesJson = json_object(Locations::instance()[SERIES]);

    leSeries->setText(series);
    if (!newSeries()) {
        const QJsonObject& object = m_seriesJson[series].toObject();
        leSeriesRus->setText(object["name_rus"].toString());
        leEmoji->setText(object["emoji"].toString());
    } else {
        leEmoji->setText("❓");
    }
    leTitle->setText(album_name);
    leTitleRus->setText(album_name);

    connect(leSeries, &QLineEdit::editingFinished, this, &CreationInfoDialog::seriesChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CreationInfoDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    seriesChanged();
}

void CreationInfoDialog::seriesChanged(){
    const bool unknownSeries = newSeries();
    lblSeriesRus->setEnabled(unknownSeries);
    leSeriesRus->setEnabled(unknownSeries);
    lblEmoji->setEnabled(unknownSeries);
    leEmoji->setEnabled(unknownSeries);
}

void CreationInfoDialog::validateAndAccept() {
    if (series().isEmpty() || title().isEmpty() || titleRus().isEmpty()) {
        QMessageBox::warning(this, "Error", "Все поля должны быть заполнены");
        return;
    }
    if (year() > QDate::currentDate().year()) {
        QMessageBox::warning(this, "Error", "Невалидный год");
        return;
    }
    if (newSeries()) {
        if (seriesRus().isEmpty() || emoji().isEmpty()) {
            QMessageBox::warning(this, "Error", "Данные о сериале должны быть заполнены");
            return;
        }
        updateSeries();
    }
    accept();
}

QString CreationInfoDialog::seriesName(const QString &title) const {
    QRegularExpression regex("^(.*?)(?=\\s\\d|\\sOVA|$)");
    auto i = regex.globalMatch(title);
    if (i.hasNext()) {
        auto match = i.peekNext();
        return match.captured(1);
    } else return title;
}

bool CreationInfoDialog::updateSeries() {
    QJsonObject series_info;
    series_info["name_rus"] = seriesRus();
    series_info["emoji"] = emoji();
    m_seriesJson[series()] = series_info;

    QFile file(Locations::instance()[SERIES]);
    if (!save_json(m_seriesJson, file)) {
        qDebug() << (QString("❌ Не удалось записать данные о сериале"));
        return false;
    }
    qDebug() << (QString("✅ Данные о сериалах записаны."));
    return true;
}

bool CreationInfoDialog::newSeries() const {
    return !m_seriesJson.contains(series());
}


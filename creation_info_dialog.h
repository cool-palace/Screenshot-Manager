#ifndef CREATION_INFO_DIALOG_H
#define CREATION_INFO_DIALOG_H

#include <QDialog>
#include <QJsonObject>
#include "ui_creation_info_dialog.h"

class CreationInfoDialog : public QDialog, public Ui_CreationInfoDialog
{
    Q_OBJECT

public:
    explicit CreationInfoDialog(const QString& album_name, QWidget *parent = nullptr);
    ~CreationInfoDialog() {}

    QString series() const      { return leSeries->text(); }
    QString seriesRus() const   { return leSeriesRus->text(); }
    QString emoji() const       { return leEmoji->text(); }
    QString title() const       { return leTitle->text(); }
    QString titleRus() const    { return leTitleRus->text(); }
    int year() const            { return sbYear->value(); }

private slots:
    void seriesChanged();
private:
    void validateAndAccept();
    QString seriesName(const QString& title) const;
    bool updateSeries();
    bool newSeries() const;

    QJsonObject m_seriesJson;
};

#endif // CREATION_INFO_DIALOG_H

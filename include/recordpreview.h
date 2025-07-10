#ifndef RECORDPREVIEW_H
#define RECORDPREVIEW_H

#include <QObject>
#include <QWidget>
#include "include\record.h"
#include "include\common.h"
#include "ui_recordpreview.h"

class TimeInputDialog : public QDialog {
    Q_OBJECT
public:
    TimeInputDialog(const QTime&, QWidget*);
    ~TimeInputDialog();
    QTime selectedTime() const { return time_edit->time(); }
private:
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QVBoxLayout layout;
    QLabel label;
    QTimeEdit* time_edit;
};

class RecordFrame : public QLabel
{
    Q_OBJECT
public:
    RecordFrame(const QString&, const QSize& size = QSize(400, 200));
    ~RecordFrame() override {}
    void set_image(const QString&, const QSize& size = QSize(400, 200));
};

class RecordPreview : public QWidget, public Ui::RecordPreview
{
    Q_OBJECT
public:
    RecordPreview(const Record&, int, const QDateTime&);
    RecordPreview(const Record&, const QDateTime&, const QStringList&, const QList<int>&);
    ~RecordPreview() override;

    int get_index() const { return index; }
    int timestamp() const { return time.toSecsSinceEpoch(); }
    QStringList tag_pair() const { return hashtags; }
    QPair<QStringList, QList<int>> get_tags() const { return qMakePair(hashtags, record_variants); }

    static QVector<Record>* records;
    static QMap<int, int>* logs;
    static QList<RecordPreview*>* selected_records;

public slots:
    void enable_reroll() { reroll_button->setEnabled(true); }
    void spinbox_changed(int value) { set_index(record_variants[value-1]); }
    void set_index(int);
    void update_log_info(int);
    void set_tagged_record(int, const QStringList&, const QList<int>&);

private slots:
    void reroll();
    void input_number();
    void switch_with_next();
    void search();
    void set_time();
    void clear();
    void update_images(const QStringList&);
    void update_text(const QString&);
    void set_tags(const QStringList&, const QList<int>&);

signals:
    void search_start(int);
    void reroll_request(RecordPreview*);

private:
    RecordPreview(const Record&, int, const QDateTime&, bool);
    int index;
    QList<QSharedPointer<RecordFrame>> images;
    QDateTime time;
    QStringList hashtags;
    QList<int> record_variants;
};

#endif // RECORDPREVIEW_H

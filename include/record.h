#ifndef RECORD_ITEMS_H
#define RECORD_ITEMS_H
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>
#include <QtConcurrent>
#include <QPushButton>
#include <QInputDialog>
#include <QTimeEdit>
#include <QDialogButtonBox>
#include "include\vk_manager.h"
#include "include\common.h"

struct Record {
    Record(const QString& quote = QString()) : quote(quote) {}
    QString quote;
    QStringList pics;
    QList<int> ids;
    QStringList links;
    bool is_public;
    QJsonObject to_json() const;
};

class RecordBase : public QWidget
{
    Q_OBJECT
public:
    RecordBase(const Record& rec = Record(), int i = 0);
    ~RecordBase() override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void update_text(const QString&);
    virtual void set_gallery_view() = 0;
    virtual void set_list_view() = 0;
    int get_index() const { return index; };
    virtual void load_thumbmnail();
signals:
    void selected(int);
protected:
    int index;
    Record record;
    QString pic;
    QLabel image;
    QLabel text;
    QLabel number;
    QGridLayout layout;
    QString path;
    QSize pic_size = QSize(160, 90);
};

class RecordItem : public RecordBase
{
    Q_OBJECT
public:
    RecordItem(const Record&, int, const QString&);
    RecordItem(const QString&, int);
    ~RecordItem() override {}
    void set_gallery_view() override;
    void set_list_view() override;
    void include_log_info(int);
private:
    int index;
    QCheckBox box;
};

class RecordTitleItem : public RecordBase
{
    Q_OBJECT
public:
    RecordTitleItem(const QString&, const QString&, int, int);
    ~RecordTitleItem() override {}
    void set_gallery_view() override {};
    void set_list_view() override {};
    void set_checked(bool);
    bool is_checked() const { return box.isChecked(); }
    QSet<int> indices() const { return title_indices; }
private:
    int size;
    QCheckBox box;
    QSet<int> title_indices;
};

class RecordFrame : public QLabel
{
    Q_OBJECT
public:
    RecordFrame(const QString&, qreal k = 1);
    ~RecordFrame() override {}
    static VK_Manager* manager;
};

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

class RecordPreview : public RecordBase
{
    Q_OBJECT
public:
    RecordPreview(const Record&, int, const QDateTime&);
    ~RecordPreview() override;
    void set_gallery_view() override {};
    void set_list_view() override;
    static QVector<Record>* records;
    static QMap<int, int>* logs;
    static QList<RecordPreview*>* selected_records;
    int timestamp() { return time.toSecsSinceEpoch(); }
    void set_index(int);
    void update_log_info(int);
signals:
    void search_start(int);
    void reroll_request(RecordPreview*);
private:
    QList<RecordFrame*> images;
    QLabel log_info;
    QDateTime time;
    QGridLayout images_layout;
    QPushButton* time_button = new QPushButton(QIcon(":/images/icons8-time-80.png"), "");
    QPushButton* reroll_button = new QPushButton(QIcon(":/images/icons8-available-updates-80.png"), "");
    QPushButton* number_button = new QPushButton(QIcon(":/images/icons8-12-80.png"), "");
    QPushButton* search_button = new QPushButton(QIcon(":/images/icons8-search-80.png"), "");
    QPushButton* switch_button = new QPushButton(QIcon(":/images/icons8-sort-down-80.png"), "");
    void reroll();
    void input_number();
    void switch_with_next();
    void search();
    void set_time();
    void clear();
    void update_images(const QStringList&);
};

#endif // RECORD_ITEMS_H

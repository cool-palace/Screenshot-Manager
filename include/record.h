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
#include <QSpinBox>
#include <QScrollArea>
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
    RecordBase(const QString&, int);
    ~RecordBase() override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void update_text(const QString&);
    virtual void set_gallery_view() = 0;
    virtual void set_list_view() = 0;
    int get_index() const { return index; }
    virtual void load_thumbmnail();
signals:
    void selected(int);
protected:
    int index;
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
    RecordTitleItem(const QString&, const QString&, int);
    ~RecordTitleItem() override {}
    void set_gallery_view() override {};
    void set_list_view() override {};
    void set_checked(bool);
    bool is_checked() const { return box.isChecked(); }
    QSet<int> indices() const { return title_indices; }
    int title_records_size() const { return size; }
private:
    int size;
    QCheckBox box;
    QSet<int> title_indices;
};

#endif // RECORD_ITEMS_H

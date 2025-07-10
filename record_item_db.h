#ifndef RECORDITEMDB_H
#define RECORDITEMDB_H
#include <record_preview_db.h>
#include <ui_record_preview_db.h>


class RecordItemDB : public QWidget, public Ui_RecordPreviewDB
{
    Q_OBJECT
public:
    explicit RecordItemDB(const RecordPreviewInfo&, QWidget *parent = nullptr);
    ~RecordItemDB() {}
    void set_record(RecordPreviewInfo && record);

signals:
    void selected(const RecordPreviewInfo&);

private slots:
    void mouseDoubleClickEvent(QMouseEvent* e);
    void update();
    void update_log_info();
    void update_images();
    void update_text();

private:
    RecordPreviewInfo m_info;
    QList<QSharedPointer<RecordFrame>> m_images;
};

#endif // RECORDITEMDB_H

#ifndef RECORDS_DIALOG_H
#define RECORDS_DIALOG_H
#include "record_item_db.h"
#include <QDialog>
#include <ui_records_dialog.h>

class RecordsDialog : public QDialog, public Ui_RecordsDialog
{
    Q_OBJECT

public:
    explicit RecordsDialog(QWidget *parent = nullptr);
    ~RecordsDialog() {}
    RecordPreviewInfo results() const { return m_result; }

private slots:
    void update();
    int offset() const { return sbLimit->value() * (sbPage->value() - 1); }

private:
    QList<RecordItemDB*> m_records;
    RecordPreviewInfo m_result;
    int m_count;
};

#endif // RECORDS_DIALOG_H

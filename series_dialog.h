#ifndef SERIES_DIALOG_H
#define SERIES_DIALOG_H

#include <QDialog>
#include <set>
#include <title_group.h>
#include <ui_series_dialog.h>

class SeriesDialog : public QDialog, public Ui_SeriesDialog
{
    Q_OBJECT

public:
    explicit SeriesDialog(const std::set<int>& included, QWidget *parent = nullptr);
    ~SeriesDialog();

public slots:
    void resizeEvent(QResizeEvent *event);

private slots:
    void check_titles(bool);
    void sort_titles();
    void lay_titles(QResizeEvent * event);

private:
    QMap<int, TitleGroup*> m_titles;
    uint8_t m_current_title_columns = 0;
};

#endif // SERIES_DIALOG_H

#ifndef JOURNALCREATION_H
#define JOURNALCREATION_H

#include <QObject>
#include "abstract_mode.h"

class JournalCreation : public AbstractPreparationMode
{
    Q_OBJECT
public:
    explicit JournalCreation(MainWindow *parent);
    QVector<int> photo_ids;
    QStringList links;

protected slots:
    virtual void keyPressEvent(QKeyEvent*) override;
    virtual void keyReleaseEvent(QKeyEvent*) override;
    virtual void slider_change(int) override;
    virtual void skip_button() override;
    virtual void add_button() override;
    virtual void back_button() override;
    virtual void ok_button() override;
    virtual void clear_all() override;
    virtual void set_enabled(bool) override;
    virtual void lay_previews(int page = 1) override {}
    virtual void draw(int) override;
    virtual void show_text(int) override;
    virtual void show_status() override;
    virtual bool data_ready() override;
    virtual void start_mode() override;
private:
    void set_albums(const QMap<QString, int>&);
    void set_photo_ids(const QVector<int>&, const QStringList&);
    bool load_albums(const QJsonObject&);
    void get_ids(const QJsonObject&);
    bool read_quote_file(QFile&);
    bool update_quote_file(const QString&);
    void register_record();
    void save_title_journal(const QString&);

};

#endif // JOURNALCREATION_H

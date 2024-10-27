#ifndef TEXT_READING_H
#define TEXT_READING_H
#include "include\abstract_mode.h"

class TextReading : public AbstractPreparationMode
{
    Q_OBJECT
public:
    explicit TextReading(MainWindow *parent);
    virtual ~TextReading();

private:
    QStringList subs;
    QRegularExpression line_regex = QRegularExpression("Dialogue: \\d+,(\\d:\\d\\d:\\d\\d\\.\\d\\d),(\\d:\\d\\d:\\d\\d\\.\\d\\d),.+,0+,0+,0+,[^,]*,({.+?})?(.+)");
    QRegularExpression timestamp_regex = QRegularExpression("(.*)?-(\\d-\\d\\d-\\d\\d-\\d{3})");

public slots:
    virtual void start() override;

protected slots:
    virtual void keyPressEvent(QKeyEvent*) override {}
    virtual void keyReleaseEvent(QKeyEvent*) override {}
    virtual void slider_change(int) override;
    virtual void skip_button() override;
    virtual void add_button() override;
    virtual void back_button() override;
    virtual void ok_button() override;
    virtual void lay_previews(int page = 1) override;
    virtual void draw(int) override;
    virtual void show_text(int) override;
    virtual void show_status() override;
    virtual bool data_ready() override;
    void set_enabled(bool);

private:
    QMultiMap<QString, QTime> timestamps_multimap();
    bool find_lines_by_timestamps(const QMultiMap<QString, QTime>&);
    bool get_subs_for_pic();
    void load_subs();
    void clear_subs();
    void update_quote_file();
};

#endif // TEXT_READING_H

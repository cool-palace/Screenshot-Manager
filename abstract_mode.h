#ifndef ABSTRACT_MODE_H
#define ABSTRACT_MODE_H
#include <QObject>
#include "mainwindow.h"
#include "ui_mainwindow.h"

enum View {
    MAIN,
    LIST,
    GALLERY,
    PREVIEW,
    TITLES
};

class AbstractMode : public QObject
{
    Q_OBJECT
public:
    explicit AbstractMode(MainWindow *parent);
    virtual ~AbstractMode();

protected:
    MainWindow* parent;
    Ui::MainWindow *ui;
    VK_Manager* manager;
    View current_view = MAIN;
    QMap<Directories, QString> locations;
    QMap<QString, int> album_ids;
    QMap<int, QString> title_map;
    QVector<Record> records;
    QList<RecordBase*> record_items;
    int pic_index = 0;
    int pic_end_index = 0;

public slots:
    virtual void start() = 0;
    virtual void keyPressEvent(QKeyEvent*) = 0;
    virtual void keyReleaseEvent(QKeyEvent*) = 0;
    virtual void closeEvent(QCloseEvent *) {}

protected slots:
    virtual void slider_change(int) = 0;
    virtual void skip_button() = 0;
    virtual void add_button() = 0;
    virtual void back_button() = 0;
    virtual void ok_button() = 0;
    virtual void lay_previews(int page = 1) = 0;
    virtual void show_status() = 0;
    virtual bool data_ready() = 0;
    virtual void set_view(View) = 0;
    void set_enabled(bool) {}

protected:
    QPixmap scaled(const QImage& source) const;
    void set_loaded_image(const QImage&);
    void clear_grid(QLayout* layout, bool hide = true);
    QString title_name(int index = 0);
};

class AbstractPreparationMode : public AbstractMode
{
    Q_OBJECT
public:
    explicit AbstractPreparationMode(MainWindow *parent);
    virtual ~AbstractPreparationMode() {}

protected:
    QStringList pics;
    QStringList quotes;
    int quote_index = 0;

protected slots:
    virtual void set_view(View) override;
    virtual void draw(int) = 0;
    virtual void show_text(int) = 0;
    bool update_quote_file(const QString&);
};

class AbstractOperationMode : public AbstractMode
{
    Q_OBJECT
public:
    explicit AbstractOperationMode(MainWindow *parent);
    virtual ~AbstractOperationMode();

protected:
    QMap<QString, HashtagButton*> hashtags;
    QList<QStringList> ranked_hashtags;
    QMap<int, QStringList> hashtags_by_index;
    QMap<QString, Hashtag> full_hashtags_map;
    QMap<int, RecordBase*> filtration_results;
    QMap<QString, FilterType> filters;
    QList<RecordBase*> title_items;
    QMap<QString, QString> title_captions;
    QMap<int, int> records_by_photo_ids;
    QMap<int, int> logs;
    QMap<int, QString> series_map;

public slots:
    void filter_event(const QString&);              // Text filters
    void filter_event(FilterType, const QString&);  // Tag filters
    void filter_event(RecordTitleItem*, bool);      // Title filters
    void filter_event(int);                         // Date filters
    void filter_event(const QMap<int, int>&);       // Logs checking

protected slots:
    virtual void set_view(View) override;
    virtual void create_hashtag_button(const QString&) = 0;
    void lay_titles();

protected:
    QRegularExpressionMatchIterator hashtag_match(const QString&) const;
    void get_hashtags();
    void update_hashtag_grid();
    void load_hashtag_info();
    void update_filters(FilterType, const QString&);
    void apply_first_filter();
    void apply_filters();
    void filter(const QSet<int>&);
    void show_filtering_results();
    void exit_filtering();
    QString filtration_indices() const;
    QString filtration_message(int) const;
    QString path(int index);
    QString series_name(int index = 0);
    void check_titles(bool);
    QPair<int, int> title_range(int);
    QSet<int> word_search(const QString&);
    QSet<int> checked_title_records();
    QSet<int> records_by_public(bool);
    QSet<int> records_by_date(int);
    QSet<int> records_by_size(FilterType);
};

#endif // ABSTRACT_MODE_H

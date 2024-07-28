#ifndef RELEASE_PREPARATION_H
#define RELEASE_PREPARATION_H
#include <QObject>
#include "include\abstract_mode.h"

class ReleasePreparation : public AbstractOperationMode
{
    Q_OBJECT
public:
    explicit ReleasePreparation(MainWindow* parent);
    virtual ~ReleasePreparation();

private:
    QMap<QString, HashtagPreview*> selected_hashtags;
    QList<RecordPreview*> selected_records;
    QMap<QString, int> poll_logs;
    QMutex status_mutex;
    int post_counter = 0;
    QMap<QString, int> series_last_used_times;
    QSet<QString> recently_posted_series;
    QMap<QString, RecordBase*> title_items_map;
    QList<QList<int>> selected_tag_pairings;
    QList<QVector<int>> hamiltonian_cycles;
    QMap<QStringList, QList<int>> smart_tag_pairs;

signals:
    void reroll_response(int);

public slots:
    virtual void start() override;
    virtual void keyPressEvent(QKeyEvent*) override;
    virtual void keyReleaseEvent(QKeyEvent*) override;

protected slots:
    virtual void set_view(View) override;
    virtual void slider_change(int) override {}
    virtual void skip_button() override {}
    virtual void add_button() override {}
    virtual void back_button() override {}
    virtual void ok_button() override {}
    virtual void lay_previews(int page = 1) override;
    virtual void show_status() override {}
    virtual void create_hashtag_button(const QString&) override;
    virtual bool data_ready() override;
    void set_enabled(bool);

private slots:
    bool open_public_journal();
    void check_logs(bool);
    void generate_button();
    void generate_release();
    void generate_release(const QVector<int>&);
    void generate_poll();
    void post_button();
    void posting_success(int, int);
    void posting_fail(int, const QString&);
    void post_poll(int);
    void poll_posting_success();
    void poll_posting_fail(const QString&);
    void poll_preparation(bool);
    void tag_pairing_analysis();
    int lowest_degree_vertex(const QList<QList<int>>& M);
    void find_hamiltonian_cycles(int current, const QList<QList<int>>& M, QVector<int>& path, QSet<int>& visited, QList<QVector<int>>& cycles, int start);
    QList<QVector<int>> get_all_hamiltonian_cycles(const QList<QList<int>>& M);

private:
    void read_poll_logs();
    void update_poll_logs();
    void update_hashtag_file();
    void change_selected_hashtag(const QString&, HashtagPreview*);
    void create_hashtag_preview_connections(const QString&);
    void create_record_preview_connections(RecordPreview*);
    void remove_hashtag_filters();
    int random_index() const;
    QString attachments(int) const;
    QString options() const;
    QString poll_message() const;
    void read_logs();
    void update_logs();
    QPair<int, int> series_range(int);
    void exclude_recently_posted_series(int);
};

#endif // RELEASE_PREPARATION_H
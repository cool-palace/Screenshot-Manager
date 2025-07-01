#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QSqlError>
#include <include/query_filters.h>

class Database : public QObject
{
    Q_OBJECT
private:
    Database();
public:
    static Database& instance();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    ~Database();

public slots:
    void init(const QString& path);
    void add_file_data(QSqlQuery& query, int record_id, const QJsonArray& filenames, const QJsonArray& links, const QJsonArray& photo_ids);
    QList<int> get_tag_ids(QSqlQuery& query, const QList<QPair<QString, int>>& tags);
    int add_series(QSqlQuery& query, const QString& series_name);
    int add_title(QSqlQuery& query, int, int, const QString&, const QString&);
    QPair<QString, QList<QPair<QString, int>>> process_caption(const QString& caption);
    void add_record(QSqlQuery& query, const QJsonObject& record, int title_id);
    void add_journal_data(QSqlQuery& query, const QJsonObject& object);
    void add_hashtags_data(QSqlQuery& query, const QJsonObject& object);
    void clear_main_tables(QSqlQuery &query);
    void clear_hashtag_table(QSqlQuery &query);
    void create_main_tables(QSqlQuery &query);
    void reset_main_tables(QSqlQuery &query);
    void reset_hashtag_table(QSqlQuery &query);
    void create_hashtag_table(QSqlQuery &query);
    void create_record_logs_table(QSqlQuery &query);
    void create_poll_logs_table(QSqlQuery &query);
    void select_records(QSqlQuery &query, const QueryFilters& filters);
    void count_records(QSqlQuery &query, const QueryFilters& filters);
    void select_series_ids(QSqlQuery &query);
    void select_excluded_series_ids(QSqlQuery &query, const QDate& date);
    void select_series_info(QSqlQuery &query);

private:
    QString select_query(const QueryFilters& filters);
    QString select_series_query(const QDate& date);

private:
    QSqlDatabase db;
};

#endif // DATABASE_H

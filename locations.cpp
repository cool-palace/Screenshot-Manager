#include "locations.h"

void Locations::init(const QJsonObject &json_file) {
    m_locations[SCREENSHOTS] = json_file.value("screenshots").toString();
    m_locations[SCREENSHOTS_NEW] = m_locations[SCREENSHOTS] + "Новые кадры\\";
    m_locations[QUOTES] = json_file.value("docs").toString();
    m_locations[SUBS] = json_file.value("subs").toString();
    m_locations[SUBS_NEW] = m_locations[SUBS] + "Новые кадры\\";
    m_locations[JOURNALS] = json_file.value("configs").toString();
    m_locations[HASHTAGS] = json_file.value("hashtags").toString();
    m_locations[LOGS_FILE] = json_file.value("logs").toString();
    m_locations[POLL_LOGS] = json_file.value("poll_logs").toString();
    m_locations[PUBLIC_RECORDS] = json_file.value("public_records").toString();
    m_locations[HIDDEN_RECORDS] = json_file.value("hidden_records").toString();
    m_locations[LABELS] = "C:\\Users\\User\\Documents\\Screenshot-Manager\\labels\\";
    m_locations[DATABASE] = "C:\\Users\\User\\Documents\\Screenshot-Manager\\your_database.db";
}

const QString Locations::operator[](Directories key) const {
    return m_locations.value(key);
}

Locations &Locations::instance() {
    static Locations instance;
    return instance;
}

#include "sqlite_storage.hpp"
#include <QtSql>

namespace {
const QString userDir = ".dekuf";
const QString dbFileName = "db.sqlite3";

bool execQuery(QSqlQuery& query)
{
    auto result = query.exec();
    // TODO: Exception instead of return code.
    if (!result)
        qDebug() << query.lastError();
    return result;
}

void migrate()
{
    QSqlQuery query;
    query.prepare("CREATE TABLE IF NOT EXISTS data_point("
                  "    id INTEGER PRIMARY KEY,"
                  "    key TEXT,"
                  "    value TEXT"
                  ")");
    execQuery(query);
}
}

SqliteStorage::SqliteStorage(const QString& databasePath)
    : db(QSqlDatabase::addDatabase("QSQLITE"))
{
    QFileInfo(databasePath).dir().mkpath(".");

    db.setDatabaseName(databasePath);
    if (!db.open())
        qDebug() << "Error: Unable to open database";
    migrate();
}

SqliteStorage::SqliteStorage()
    : SqliteStorage(QDir::homePath() + QDir::separator() + userDir
        + QDir::separator() + dbFileName)
{
}

QList<QString> SqliteStorage::listDataPoints(const QString& key)
{
    QList<QString> dataPoints;
    QSqlQuery query;
    query.prepare("SELECT value FROM data_point WHERE key = :key");
    query.bindValue(":key", key);
    if (!execQuery(query))
        return dataPoints;

    while (query.next())
        dataPoints.push_back(query.value(0).toString());
    return dataPoints;
}

void SqliteStorage::addDataPoint(const QString& key, const QString& value)
{
    QSqlQuery query;
    query.prepare("INSERT INTO data_point (key, value) values (:key, :value)");
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    execQuery(query);
}

#pragma once

#include <QtCore>

#include "sqlite_storage.hpp"
#include "survey.hpp"
#include "survey_response.hpp"
class QNetworkAccessManager;

class Client : public QObject {
    Q_OBJECT

public:
    explicit Client(QObject* parent = 0);
    void fetchSurveys();

public slots:
    void run();

signals:
    void finished();

private:
    SqliteStorage storage;
    QNetworkAccessManager* manager;

    void handleSurveysResponse(const QByteArray& data);
    void postSurveyResponse(QSharedPointer<SurveyResponse>);
};

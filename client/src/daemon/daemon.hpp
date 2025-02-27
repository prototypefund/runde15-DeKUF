#pragma once

#include <QtCore>

#include <core/storage.hpp>
#include <core/survey_response.hpp>

#include "core/survey.hpp"
#include "dbus_service.hpp"
#include "encryption.hpp"
#include "network.hpp"

class Daemon : public QObject {
    Q_OBJECT

    friend class DaemonTest;

public:
    Daemon(QObject* parent, QSharedPointer<Storage> storage,
        QSharedPointer<Network> network, QSharedPointer<Encryption> encryption);

public slots:
    void run();

signals:
    void finished();

private:
    QSharedPointer<Storage> storage;
    QSharedPointer<Network> network;
    QSharedPointer<Encryption> encryption;
    DBusService dbusService;

    bool checkIfAllDataKeysArePresent(
        const QSharedPointer<Survey>& survey) const;
    void handleSurveysResponse(const QByteArray& data);
    void processSurveys();
    void processInitialSignup(SurveyRecord& record);
    void processSignups();
    void postMessageToDelegate(SurveyRecord& record) const;
    void processMessagesForDelegate(SurveyRecord& record);
    QSharedPointer<SurveyResponse> createSurveyResponse(
        const QSharedPointer<Survey>&) const;
    QSharedPointer<QueryResponse> createQueryResponse(
        const QSharedPointer<Query>& query) const;
    Result<QList<QSharedPointer<EncryptedSurveyResponse>>>
    parseResponseMessages(const QByteArray& data, int groupSize) const;
    void signUpForSurvey(const QSharedPointer<const Survey> survey);
};

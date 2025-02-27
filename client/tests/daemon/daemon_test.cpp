#include <QTest>

#include <core/survey_response.hpp>
#include <daemon/daemon.hpp>

#include "../stubs/core/storage_stub.hpp"
#include "../stubs/daemon/network_stub.hpp"

#include "daemon_test.hpp"

#include "daemon/identity_encryption.hpp"

void DaemonTest::testProcessSurveysIgnoresErrors()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);
    daemon.processSurveys();
    QCOMPARE(storage->listSurveyRecords().count(), 0);
}

void DaemonTest::testProcessSurveysSignsUpForRightCommissioner()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    survey.commissioner = QSharedPointer<Commissioner>::create("KDE");
    network->listSurveysResponse
        = QByteArray("[\n" + survey.toByteArray() + "\n]");

    daemon.processSurveys();
    auto records = storage->listSurveyRecords();
    QCOMPARE(records.count(), 1);
    auto first = records.first();
    QCOMPARE(first.survey->id, survey.id);
}

void DaemonTest::testProcessSurveyDoesNotSignUpForWrongCommissioner()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    survey.commissioner = QSharedPointer<Commissioner>::create("Wrong");
    network->listSurveysResponse
        = QByteArray("[\n" + survey.toByteArray() + "\n]");

    daemon.processSurveys();
    QCOMPARE(storage->listSurveyRecords().count(), 0);
}

void DaemonTest::testProcessSurveyDoesNotSignUpForWhenDataKeyNotPresent()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    survey.queries.push_back(QSharedPointer<Query>::create(
        "1", "unknownKey", QList<QString> { "1" }, true));
    survey.commissioner = QSharedPointer<Commissioner>::create("KDE");
    network->listSurveysResponse
        = QByteArray("[\n" + survey.toByteArray() + "\n]");

    daemon.processSurveys();
    QCOMPARE(storage->listSurveyRecords().count(), 0);
}

void DaemonTest::testProcessSignupsIgnoresEmptySignupState()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(
        survey, "1337", "", "", std::nullopt, std::nullopt);

    daemon.processSignups();
    QCOMPARE(
        storage->listSurveyRecords().first().getState(), SurveyState::Initial);
}

void DaemonTest::testProcessSignupsIgnoresNonStartedAggregations()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(
        survey, "1337", "", "", std::nullopt, std::nullopt);

    network->getSignupStateResponse = QByteArray(R"({
        "aggregation_started": false,
        "delegate_public_key": "1337",
        "group_size": 1,
        "aggregation_public_key_n": "123"
    })");

    daemon.processSignups();
    QCOMPARE(
        storage->listSurveyRecords().first().getState(), SurveyState::Initial);
}

void DaemonTest::testProcessSignupsHandlesDelegateCase()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(
        survey, "1", "1337", "", std::nullopt, std::nullopt);

    network->getSignupStateResponse = QByteArray(R"({
        "aggregation_started": true,
        "delegate_public_key": "1337",
        "group_size": 1,
        "aggregation_public_key_n": "123"
    })");

    daemon.processSignups();
    auto records = storage->listSurveyRecords();
    QCOMPARE(records.count(), 1);
    auto first = records.first();
    QCOMPARE(first.delegatePublicKey, "1337");
    QCOMPARE(first.getState(), SurveyState::Processing);
    QCOMPARE(first.groupSize, 1);
}

void DaemonTest::testProcessSignupsHandlesNonDelegateCase()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(survey, "1", "1337", "", "123", std::nullopt);

    network->getSignupStateResponse = QByteArray(R"({
        "aggregation_started": true,
        "delegate_public_key": "2448",
        "group_size": 1,
        "aggregation_public_key_n": "123"
    })");

    daemon.processSignups();
    auto records = storage->listSurveyRecords();
    QCOMPARE(records.count(), 1);
    auto first = records.first();
    QCOMPARE(first.delegatePublicKey, "2448");

    // TODO: The state should actually be Done here, but since we don't actually
    // submit a response yet, it's not.
    QCOMPARE(first.getState(), SurveyState::Initial);
}

void DaemonTest::testProcessSignupsIgnoresEmptyMessagesForDelegate()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(
        survey, "1337", "1337", "1337", std::nullopt, std::nullopt);
    daemon.processSignups();
}

// TODO: Instead of testing createSurveyResponse directly, it'd be better to
//       rewrite the following test processSignups.

void DaemonTest::testCreateSurveyResponseSucceedsForIntervals()
{
    auto storage = QSharedPointer<StorageStub>::create();

    Survey survey("testId", "testName");
    QList<QString> cohorts = { "[8, 16)", "[16, 32)", "[32, 64)" };
    survey.queries.append(
        QSharedPointer<Query>::create("1", "testKey", cohorts, false));
    survey.commissioner = QSharedPointer<Commissioner>::create("KDE");
    storage->addDataPoint("testKey", "8");
    storage->addDataPoint("testKey", "16");
    storage->addDataPoint("testKey", "31");

    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);
    const auto surveyResponse
        = daemon.createSurveyResponse(QSharedPointer<Survey>::create(survey));

    QMap<QString, int> testCohortData
        = { { "[8, 16)", 1 }, { "[16, 32)", 2 }, { "[32, 64)", 0 } };

    QVERIFY(!surveyResponse.isNull());
    QCOMPARE(surveyResponse->queryResponses.first()->queryId, "1");
    QCOMPARE(
        surveyResponse->queryResponses.first()->cohortData, testCohortData);
}

void DaemonTest::testCreateSurveyResponseSucceedsForIntervalsWithInfinity()
{
    auto storage = QSharedPointer<StorageStub>::create();

    Survey survey("testId", "testName");
    QList<QString> cohorts = { "(-inf, 16)", "[16, 32)", "[32, inf)" };
    survey.queries.append(
        QSharedPointer<Query>::create("1", "testKey", cohorts, false));
    survey.commissioner = QSharedPointer<Commissioner>::create("KDE");
    storage->addDataPoint("testKey", "8");
    storage->addDataPoint("testKey", "31");
    storage->addDataPoint("testKey", "32");
    storage->addDataPoint("testKey", "1000");
    storage->addDataPoint("testKey", "10000.23");

    auto network = QSharedPointer<NetworkStub>::create();
    auto encryption = QSharedPointer<IdentityEncryption>::create();
    Daemon daemon(nullptr, storage, network, encryption);
    const auto surveyResponse
        = daemon.createSurveyResponse(QSharedPointer<Survey>::create(survey));

    QMap<QString, int> testCohortData
        = { { "(-inf, 16)", 1 }, { "[16, 32)", 1 }, { "[32, inf)", 3 } };

    QVERIFY(!surveyResponse.isNull());
    QCOMPARE(surveyResponse->queryResponses.first()->queryId, "1");
    QCOMPARE(
        surveyResponse->queryResponses.first()->cohortData, testCohortData);
}

QTEST_MAIN(DaemonTest)

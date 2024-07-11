#include <QSignalSpy>
#include <QTest>

#include <core/survey_response.hpp>
#include <daemon/daemon.hpp>

#include "../stubs/core/storage_stub.hpp"
#include "../stubs/daemon/network_stub.hpp"

#include "daemon_test.hpp"

namespace {
template <typename T>
void await(const QFuture<T>& future, const int timeout = 250)
{
    QFutureWatcher<T> watcher;
    watcher.setFuture(future);
    QSignalSpy spy(&watcher, &QFutureWatcher<void>::finished);
    QVERIFY2(spy.wait(timeout), "Future never finished");
}
};

void DaemonTest::testProcessSurveysIgnoresErrors()
{
    auto storage = QSharedPointer<StorageStub>::create();
    Daemon daemon(nullptr, storage, QSharedPointer<NetworkStub>::create());
    await(daemon.processSurveys());
    QCOMPARE(storage->listSurveyRecords().count(), 0);
}

void DaemonTest::testProcessSurveysSignsUpForRightCommissioner()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    Daemon daemon(nullptr, storage, network);

    Survey survey("testId", "testName");
    survey.commissioner = QSharedPointer<Commissioner>::create("KDE");
    network->listSurveysResponse
        = QByteArray("[\n" + survey.toByteArray() + "\n]");

    await(daemon.processSurveys());
    auto records = storage->listSurveyRecords();
    QCOMPARE(records.count(), 1);
    auto first = records.first();
    QCOMPARE(first.survey->id, survey.id);
}

void DaemonTest::testProcessSurveyDoesNotSignUpForWrongCommissioner()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    Daemon daemon(nullptr, storage, network);

    Survey survey("testId", "testName");
    survey.commissioner = QSharedPointer<Commissioner>::create("Wrong");
    network->listSurveysResponse
        = QByteArray("[\n" + survey.toByteArray() + "\n]");

    await(daemon.processSurveys());
    QCOMPARE(storage->listSurveyRecords().count(), 0);
}

void DaemonTest::testProcessSignupsIgnoresEmptySignupState()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    Daemon daemon(nullptr, storage, network);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(survey, "1337", "", std::nullopt);

    await(daemon.processSignups());
    QCOMPARE(
        storage->listSurveyRecords().first().getState(), SurveyState::Initial);
}

void DaemonTest::testProcessSignupsIgnoresNonStartedAggregations()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    Daemon daemon(nullptr, storage, network);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(survey, "1337", "", std::nullopt);

    network->getSignupStateResponse = QByteArray(R"({
        "aggregation_started": false,
        "delegate_id": "1337",
        "group_size": 1
    })");

    await(daemon.processSignups());
    QCOMPARE(
        storage->listSurveyRecords().first().getState(), SurveyState::Initial);
}

void DaemonTest::testProcessSignupsHandlesDelegateCase()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    Daemon daemon(nullptr, storage, network);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(survey, "1337", "", std::nullopt);

    network->getSignupStateResponse = QByteArray(R"({
        "aggregation_started": true,
        "delegate_id": "1337",
        "group_size": 1
    })");

    await(daemon.processSignups());
    auto records = storage->listSurveyRecords();
    QCOMPARE(records.count(), 1);
    auto first = records.first();
    QCOMPARE(first.delegateId, "1337");
    QCOMPARE(first.getState(), SurveyState::Processing);
    QCOMPARE(first.groupSize, 1);
}

void DaemonTest::testProcessSignupsHandlesNonDelegateCase()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    Daemon daemon(nullptr, storage, network);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(survey, "1337", "", std::nullopt);

    network->getSignupStateResponse = QByteArray(R"({
        "aggregation_started": true,
        "delegate_id": "2448",
        "group_size": 1
    })");

    await(daemon.processSignups());
    auto records = storage->listSurveyRecords();
    QCOMPARE(records.count(), 1);
    auto first = records.first();
    QCOMPARE(first.delegateId, "2448");

    // TODO: The state should actually be Done here, but since we don't actually
    // submit a response yet, it's not.
    QCOMPARE(first.getState(), SurveyState::Initial);
}

void DaemonTest::testProcessSignupsIgnoresEmptyMessagesForDelegate()
{
    auto storage = QSharedPointer<StorageStub>::create();
    auto network = QSharedPointer<NetworkStub>::create();
    Daemon daemon(nullptr, storage, network);

    Survey survey("testId", "testName");
    storage->addSurveyRecord(survey, "1337", "1337", std::nullopt);
    await(daemon.processSignups());
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

    Daemon daemon(nullptr, storage, QSharedPointer<NetworkStub>::create());
    const auto surveyResponse = daemon.createSurveyResponse(survey);

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

    Daemon daemon(nullptr, storage, QSharedPointer<NetworkStub>::create());
    const auto surveyResponse = daemon.createSurveyResponse(survey);

    QMap<QString, int> testCohortData
        = { { "(-inf, 16)", 1 }, { "[16, 32)", 1 }, { "[32, inf)", 3 } };

    QVERIFY(!surveyResponse.isNull());
    QCOMPARE(surveyResponse->queryResponses.first()->queryId, "1");
    QCOMPARE(
        surveyResponse->queryResponses.first()->cohortData, testCohortData);
}

QTEST_MAIN(DaemonTest)

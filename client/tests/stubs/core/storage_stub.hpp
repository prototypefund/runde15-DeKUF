#pragma once

#include <QtCore>
#include <core/storage.hpp>

class StorageStub : public Storage {
private:
    QList<QPair<QString, QString>> dataPoints;
    QList<SurveyResponseRecord> surveyResponses;
    QList<SurveySignup> surveySignups;

public:
    QList<DataPoint> listDataPoints(const QString& key) const
    {
        QList<DataPoint> matchingValues;
        for (const auto& dataPoint : dataPoints)
            matchingValues.push_back({ .value = dataPoint.second });
        return matchingValues;
    };

    void addDataPoint(const QString& dataKey, const QString& data)
    {
        dataPoints.push_back(QPair<QString, QString>(dataKey, data));
    }

    QList<SurveyResponseRecord> listSurveyResponses() const
    {
        return surveyResponses;
    }

    void addSurveyResponse(const SurveyResponse& response, const Survey& survey)
    {
        surveyResponses.push_back(
            { .response = QSharedPointer<SurveyResponse>::create(response),
                .createdAt = QDateTime::currentDateTime() });
    }

    QList<SurveySignup> listSurveySignups() const { return surveySignups; }

    QList<SurveySignup> listSurveySignupsForState(const QString& state) const
    {
        QList<SurveySignup> signups;
        for (auto signup : listSurveySignups())
            if (signup.state == state)
                signups.append(signup);
        return signups;
    }

    QList<SurveySignup> listActiveDelegateSurveySignups() const
    {
        QList<SurveySignup> signups;
        for (auto signup : listSurveySignups())
            if (signup.state == "processing"
                && signup.clientId == signup.delegateId)
                signups.append(signup);
        return signups;
    }

    void addSurveySignup(const Survey& survey, const QString& state,
        const QString& clientId, const QString& delegateId)
    {
        surveySignups.push_back(
            { .survey = QSharedPointer<Survey>::create(survey),
                .state = state,
                .clientId = clientId,
                .delegateId = delegateId });
    }

    void saveSurveySignup(const SurveySignup& signup)
    {
        for (auto& existingSignup : surveySignups) {
            if (existingSignup.clientId != signup.clientId)
                continue;
            existingSignup.state = signup.state;
            existingSignup.delegateId = signup.delegateId;
            existingSignup.groupSize = signup.groupSize;
            break;
        }
    }
};

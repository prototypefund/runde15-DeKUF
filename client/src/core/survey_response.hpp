#pragma once

#include <QtCore>

#include "commissioner.hpp"
#include "survey_response.hpp"

class QueryResponse {
public:
    const QString dataKey;
    const QString data;

    QueryResponse(const QString& dataKey, const QString& data);

    bool operator==(const QueryResponse& other) const
    {
        return dataKey == other.dataKey && data == other.data;
    }
};

class SurveyResponse {
public:
    static QSharedPointer<SurveyResponse> fromJsonByteArray(
        const QByteArray& responseData);

    QList<QSharedPointer<Commissioner>> commissioners;
    QList<QSharedPointer<QueryResponse>> queryResponses;

    QByteArray toJsonByteArray() const;
};

import json

from core.json_serializers import SurveyResponseSerializer, SurveySerializer
from core.models.aggregation_group import AggregationGroup
from core.models.client_to_delegate_message import ClientToDelegateMessage
from core.models.grouping_logic import group_ungrouped_signups
from core.models.survey import Survey
from core.models.survey_signup import SurveySignup
from django.http import HttpResponse, HttpResponseBadRequest, JsonResponse
from django.shortcuts import get_object_or_404
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_http_methods
from rest_framework.parsers import JSONParser


@csrf_exempt
@require_http_methods(["POST"])
def save_survey_response(request):
    try:
        survey_response_data = JSONParser().parse(request)

        print(survey_response_data)
        serializer = SurveyResponseSerializer(data=survey_response_data)

        if serializer.is_valid():
            survey_response = serializer.save()
            for query_response in survey_response.query_responses.all():
                query_response.query.aggregate_query_response(query_response)
            return JsonResponse(serializer.data, status=201)
        else:
            return JsonResponse(serializer.errors, status=400)
    except json.JSONDecodeError:
        return HttpResponse("Invalid JSON", status=400)


@require_http_methods(["GET"])
def get_surveys(request):
    surveys = Survey.objects.all()
    serializer = SurveySerializer(surveys, many=True)
    return JsonResponse(serializer.data, safe=False)


@csrf_exempt
@require_http_methods(["POST"])
def signup_to_survey(request, survey_id):
    try:
        survey = get_object_or_404(Survey, id=survey_id)
        survey_signup = SurveySignup.objects.create(survey=survey)

        response_data = {
            "client_id": str(survey_signup.id),
            "survey_id": str(survey.id),
            "time": survey_signup.time.isoformat(),
        }

        group_ungrouped_signups(
            ungrouped_signups=list(
                SurveySignup.objects.filter(group__isnull=True)
            ),
            survey=survey,
        )

        return JsonResponse(response_data, status=201)
    except json.JSONDecodeError:
        return HttpResponse("Invalid JSON", status=400)


@csrf_exempt
@require_http_methods(["GET"])
def get_signup_state(request, client_id):
    try:
        survey_signup = get_object_or_404(SurveySignup, id=client_id)
        aggregation_group = survey_signup.group

        if not aggregation_group:
            response_data = {"delegate_id": "", "aggregation_started": False}
            return JsonResponse(response_data, status=200)

        delegate = aggregation_group.delegate

        response_data = {
            "delegate_id": str(delegate.id),
            "aggregation_started": True,
        }

        return JsonResponse(response_data, status=200)
    except json.JSONDecodeError:
        return HttpResponse("Invalid JSON", status=400)


@csrf_exempt
@require_http_methods(["POST"])
def message_to_delegate(request, delegate_id):
    aggregation_group = get_object_or_404(
        AggregationGroup, delegate=delegate_id
    )
    if request.body is None:
        return HttpResponse("Message content required", status=400)

    try:
        body_data = json.loads(request.body.decode("utf-8"))
    except json.JSONDecodeError:
        return HttpResponseBadRequest("Invalid JSON")

    if not isinstance(body_data, dict):
        return HttpResponseBadRequest("Invalid data format")

    ClientToDelegateMessage.objects.create(
        delegate=delegate_id, group=aggregation_group, content=body_data
    )

    return HttpResponse(status=201)


@csrf_exempt
@require_http_methods(["GET"])
def get_messages_for_delegate(request, delegate_id):

    messages = ClientToDelegateMessage.objects.filter(delegate=delegate_id)

    if len(messages) == 0:
        return HttpResponseBadRequest(f"No messages found for: {delegate_id}")

    return JsonResponse(
        {"messages": [message.content for message in messages]}, status=200
    )


@csrf_exempt
@require_http_methods(["POST"])
def post_aggregation_result(request, delegate_id):

    aggregation_group = AggregationGroup.objects.get(delegate=delegate_id)

    if not aggregation_group:
        return HttpResponseBadRequest(
            f"No aggregation group found for: {delegate_id}"
        )

    try:
        survey_response_data = JSONParser().parse(request)

        serializer = SurveyResponseSerializer(data=survey_response_data)

        if serializer.is_valid():
            survey_response = serializer.save()
            for query_response in survey_response.query_responses.all():
                query_response.query.aggregate_query_response(query_response)

            SurveySignup.objects.filter(group=aggregation_group).delete()
            aggregation_group.delete()
            return JsonResponse(serializer.data, status=201)
        else:
            return JsonResponse(serializer.errors, status=400)
    except json.JSONDecodeError:
        return HttpResponse("Invalid JSON", status=400)

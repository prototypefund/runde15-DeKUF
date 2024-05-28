"""
URL configuration for dekufserver project.

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/5.0/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""

from core.views import get_surveys, save_survey_response, signup_to_survey
from django.contrib import admin
from django.urls import path

urlpatterns = [
    path("admin/", admin.site.urls),
    path("api/survey-response/", save_survey_response, name="survey-response"),
    path("api/surveys/", get_surveys, name="get-surveys"),
    path(
        "api/survey-signup/<uuid:survey_id>/",
        signup_to_survey,
        name="survey-signup",
    ),
]

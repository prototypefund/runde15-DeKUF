# Generated by Django 5.0.3 on 2024-03-07 17:15

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ("core", "0002_commissioner_query_queryresponse_survey_and_more")
    ]

    operations = [
        migrations.AddField(
            model_name="survey",
            name="name",
            field=models.CharField(default="name", max_length=200),
            preserve_default=False,
        )
    ]

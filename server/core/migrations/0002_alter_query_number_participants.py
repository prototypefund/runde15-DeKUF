# Generated by Django 4.2.11 on 2024-05-06 15:35

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [("core", "0001_initial")]

    operations = [
        migrations.AlterField(
            model_name="query",
            name="number_participants",
            field=models.IntegerField(default=0, editable=False),
        )
    ]

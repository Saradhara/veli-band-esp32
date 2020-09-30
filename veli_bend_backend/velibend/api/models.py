from django.db import models


class RegisterDevice(models.Model):
    uuid = models.CharField(max_length=36, primary_key=True)
    employee_id = models.CharField(max_length=36)
    employee_name = models.CharField(max_length=36)
    temperature = models.IntegerField()
    breathing = models.BooleanField()
    cough = models.BooleanField()
    body_pain = models.BooleanField()
    throat_pain = models.BooleanField()
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    def __str__(self):
        return "{}, {}, {}".format(self.uuid, self.employee_id, self.employee_name)


class TraceEvent(models.Model):
    partner_uuid = models.CharField(max_length=36)
    self_uuid = models.CharField(max_length=36)
    self_emp_id = models.CharField(max_length=36)
    self_emp_name = models.CharField(max_length=36)
    partner_emp_id = models.CharField(max_length=36)
    partner_emp_name = models.CharField(max_length=36)
    max_distance = models.FloatField()
    min_distance = models.FloatField()
    duration = models.IntegerField()
    SEVERITY = (
        ('HIGH_RISK', 'HIGH RISK'),
        ('MEDIUM_RISK', 'MEDIUM_RISK'),
        ('LOW_RISK', 'LOW_RISK'),
    )
    covid_risk = models.CharField(max_length=8, choices=SEVERITY)
    first_timestamp = models.DateTimeField(default=None)
    last_timestamp = models.DateTimeField(default=None)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    def __str__(self):
        return "{}, {}, {}".format(self.pk, self.partner_uuid, self.self_uuid)

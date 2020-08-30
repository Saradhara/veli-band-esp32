from rest_framework import serializers

from .models import TraceEvent
from .models import RegisterDevice


class TraceEventSerializer(serializers.ModelSerializer):
    class Meta:
        model = TraceEvent
        fields = '__all__'


class RegisterDeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = RegisterDevice
        fields = '__all__'

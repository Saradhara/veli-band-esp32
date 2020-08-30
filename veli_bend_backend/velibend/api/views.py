import logging

from django.shortcuts import get_object_or_404
from rest_framework import status, viewsets
from rest_framework.response import Response
from django.db.models import Q
from .models import RegisterDevice
from .models import TraceEvent
from .serializers import RegisterDeviceSerializer
from .serializers import TraceEventSerializer

logger = logging.getLogger(__name__)
active_traces = {}

PARTNER_UUID = 'partner_uuid'
SELF_UUID = 'self_uuid'
COVID_RISK = 'covid_risk'
MAX_DISTANCE = 'max_distance'
MIN_DISTANCE = 'min_distance'
FIRST_TIMESTAMP = 'first_timestamp'
LAST_TIMESTAMP = 'last_timestamp'
DURATION = 'duration'
IP = 'ip'
BATTERY = 'battery'


class TraceEventsViewSet(viewsets.ModelViewSet):
    # permission_classes = (IsAuthenticated,)
    queryset = TraceEvent.objects.all()
    serializer_class = TraceEventSerializer

    def create(self, request, *args, **kwargs):
        logging.info("trace event : {}".format(request.data))
        trace_event = request.data
        self_uuid = request.data.get(SELF_UUID)
        partner_uuid = request.data.get(PARTNER_UUID)
        self_device = get_object_or_404(RegisterDevice, pk=self_uuid)
        partner_device = get_object_or_404(RegisterDevice, pk=partner_uuid)
        trace_event.update({
            'self_emp_id': self_device.employee_id,
            'self_emp_name': self_device.employee_name,
            'partner_emp_id': partner_device.employee_id,
            'partner_emp_name': partner_device.employee_name
        })
        serializer = self.get_serializer(data=trace_event, many=isinstance(trace_event, list))
        serializer.is_valid(raise_exception=True)
        self.perform_create(serializer)
        headers = self.get_success_headers(serializer.data)
        return Response(serializer.data, status=status.HTTP_201_CREATED, headers=headers)

    def list(self, request, **kwargs):
        emp_id = self.request.query_params.get('emp_id')
        duration_gt = self.request.query_params.get('duration_gt')
        covid_risk = self.request.query_params.get('covid_risk')
        start_date = self.request.query_params.get('start_date')
        end_date = self.request.query_params.get('end_date')
        queryset = self.get_queryset()
        if emp_id:
            queryset = queryset.filter(Q(self_emp_id=emp_id) | Q(partner_emp_id=emp_id))
        if duration_gt:
            queryset = queryset.filter(duration__gte=int(duration_gt))
        if covid_risk:
            queryset = queryset.filter(covid_risk=covid_risk)
        if start_date and end_date:
            queryset = queryset.filter(created_at__range=(start_date, end_date))

        serializer = TraceEventSerializer(queryset, many=True)
        return Response(serializer.data)


class ActiveTraceEventsViewSet(viewsets.ViewSet):

    def create(self, request):
        global active_traces
        active_traces = {}
        for trace in request.data:
            self_uuid = trace.get(SELF_UUID)
            partner_uuid = trace.get(PARTNER_UUID)
            try:
                self_device = RegisterDevice.objects.get(pk=self_uuid)
                partner_device = RegisterDevice.objects.get(pk=partner_uuid)
                trace.update({
                    'self_emp_id': self_device.employee_id,
                    'self_emp_name': self_device.employee_name,
                    'partner_emp_id': partner_device.employee_id,
                    'partner_emp_name': partner_device.employee_name
                })
                trace_uuid = "{}@{}".format(trace.get(SELF_UUID), trace.get(PARTNER_UUID))
                alternate_uuid = "{}@{}".format(trace.get(PARTNER_UUID), trace.get(SELF_UUID))
                if trace_uuid not in active_traces and alternate_uuid not in active_traces:
                    active_traces[trace_uuid] = trace
                else:
                    trace_uuid = trace_uuid if trace_uuid in active_traces else alternate_uuid
                    active_traces[trace_uuid] = trace
            except RegisterDevice.DoesNotExist:
                logger.debug("Device is not registered ...so not adding to active traces")
        return Response(active_traces, status=status.HTTP_201_CREATED)

    def list(self, request):
        global active_traces
        return Response(active_traces, status=status.HTTP_200_OK)


class RegisterDeviceViewSet(viewsets.ModelViewSet):
    # permission_classes = (IsAuthenticated,)
    queryset = RegisterDevice.objects.all()
    serializer_class = RegisterDeviceSerializer

    def create(self, request, *args, **kwargs):
        print(request.data)
        serializer = self.get_serializer(data=request.data, many=isinstance(request.data, list))
        serializer.is_valid(raise_exception=True)
        self.perform_create(serializer)
        headers = self.get_success_headers(serializer.data)
        return Response(serializer.data, status=status.HTTP_201_CREATED, headers=headers)

    def list(self, request, **kwargs):
        queryset = self.get_queryset()
        serializer = self.get_serializer(queryset, many=True)
        return Response(serializer.data)


class MedicalHealthViewSet(viewsets.ModelViewSet):
    # permission_classes = (IsAuthenticated,)
    queryset = RegisterDevice.objects.all()
    serializer_class = RegisterDeviceSerializer

    def list(self, request, **kwargs):
        queryset = self.get_queryset()
        cough_reported = queryset.filter(cough=True).values()
        breathing_reported = queryset.filter(breathing=True).values()
        body_pain_reported = queryset.filter(body_pain=True).values()
        throat_pain_reported = queryset.filter(throat_pain=True).values()
        temperature_reported = queryset.filter(temperature__gte=100.4).values()
        medical_health_status = {
            'cough_details': {
                'cough_reported': len(cough_reported),
                'data': list(cough_reported)
            },
            'breathing_details': {
                'breathing_reported': len(breathing_reported),
                'data': list(breathing_reported)
            },
            'body_pain_details': {
                'body_pain_reported': len(body_pain_reported),
                'data': list(body_pain_reported)
            },
            'throat_pain_details': {
                'throat_pain_reported': len(throat_pain_reported),
                'data': list(throat_pain_reported)
            },
            'temperature_details': {
                'temperature_reported': len(temperature_reported),
                'data': list(temperature_reported)
            }

        }
        return Response(medical_health_status, status=status.HTTP_200_OK)

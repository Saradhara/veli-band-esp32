from django.urls import include, path
from rest_framework import routers

from . import views

router = routers.DefaultRouter()
router.register(r'trace-events', views.TraceEventsViewSet)
router.register(r'register-devices', views.RegisterDeviceViewSet)
router.register(r'active-traces', views.ActiveTraceEventsViewSet, basename='active-traces')
router.register(r'medical-health', views.MedicalHealthViewSet, basename='medical-health')

urlpatterns = [
    path('', include(router.urls)),
    path('api-auth/', include('rest_framework.urls', namespace='rest_framework'))
]

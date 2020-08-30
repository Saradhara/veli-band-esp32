from django.contrib import admin
from .models import TraceEvent
from .models import RegisterDevice

# Register your models here.
admin.site.register(TraceEvent)
admin.site.register(RegisterDevice)

import logging

from config import Constants

log = logging.getLogger(Constants.LOG_HANDLER_NAME)


class HeartBeatTracker:
    def __init__(self):
        self.registered_devices = {}

    def add(self, self_uuid, ip, battery, timestamp):
        self.registered_devices.update({self_uuid: {
            Constants.SELF_UUID: self_uuid,
            Constants.IP: ip,
            Constants.LAST_TIMESTAMP: timestamp,
            Constants.BATTERY: battery
        }
        })

    def get_registered_devices(self):
        return self.registered_devices

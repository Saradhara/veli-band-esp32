import logging
import os


class Constants:
    """
    This class is responsible for configuration constants
    """

    def __init__(self):
        pass

    TRACE_TOPIC_ID = 'trace'
    HEARTBEAT_TOPIC_ID = 'heartbeat'
    # log configuration
    CENTRAL_MQTT_BROKER = 'localhost'
    LOG_HANDLER_NAME = 'trace_logger'
    ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
    LOGGING_DIR = 'log/'

    LOG_LEVEL = logging.INFO

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
    ACTIVE_TRACES_API_ENDPOINT = 'http://localhost:8000/active-traces/'
    COMPLETED_TRACES_API_ENDPOINT = 'http://localhost:8000/trace-events/'

    CHK_TIMER = 2
    THRESHOLD = 15

from __future__ import division

import json
import logging
import threading
import time
from datetime import datetime
from queue import Queue

import paho.mqtt.client as mqtt
import requests

from config import Constants
from heart_beat_tracker import HeartBeatTracker
from logger import Logger
from trace_tracker import TraceTracker

log = logging.getLogger(Constants.LOG_HANDLER_NAME)
trace_tracker = TraceTracker(chk_timer=Constants.CHK_TIMER, threshold=Constants.THRESHOLD)
heart_beat_tracker = HeartBeatTracker()
active_traces_q = Queue()
completed_traces_q = Queue()


def on_connect(client, userdata, t1, t2):
    log.info("connected to broker......")
    client.subscribe(Constants.TRACE_TOPIC_ID)
    log.info("you have subscribe for the topic {}".format(Constants.TRACE_TOPIC_ID))
    client.subscribe(Constants.HEARTBEAT_TOPIC_ID)
    log.info("you have subscribe for the topic {}".format(Constants.HEARTBEAT_TOPIC_ID))


def on_message(client, userdata, msg):
    if Constants.HEARTBEAT_TOPIC_ID == msg.topic:
        obj = json.loads(msg.payload.decode('utf-8'))  # decode json string
        self_uuid = "{}".format(obj[Constants.SELF_UUID])
        ip = "{}".format(obj[Constants.IP])
        battery = int(obj[Constants.BATTERY])
        timestamp = int(time.time())
        heart_beat_tracker.add(self_uuid, ip, battery, timestamp)
    if Constants.TRACE_TOPIC_ID == msg.topic:
        obj = json.loads(msg.payload.decode('utf-8'))  # decode json string
        self_uuid = "{}".format(obj['self_uuid'])
        partner_uuid = "{}".format(obj['partner_uuid'])
        covid_risk = "{}".format(obj['covid_risk'])
        distance = round(float("{}".format(obj['distance'])), 2)
        timestamp = int(time.time())
        trace_tracker.add(self_uuid=self_uuid, partner_uuid=partner_uuid, covid_risk=covid_risk, distance=distance,
                          timestamp=timestamp)


def init_mqtt(url="localhost", port=1883, keep_alive=60):
    """Init MQTT connection"""
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    try:
        print(url, port, keep_alive)
        client.connect(url, port, keep_alive)
        client.loop_start()
        return client
    except Exception as e:
        log.exception(e)
        return None


def push_data_to_mqtt_broker(mqtt_client, data, topic_name):
    log.info("publishing message to mqtt broker with topic {}".format(topic_name))
    mqtt_client.publish(topic=topic_name, payload=str(data))
    log.info("Message ::{}".format(data))
    log.info("Message Published Successfully")


def push_data_to_backend(trace_events, api):
    payload = json.dumps(trace_events)
    headers = {
        'content-type': "application/json",
        'cache-control': "no-cache",
    }
    try:
        response = requests.request("POST", api, data=payload, headers=headers)
        print("Data pushed to API {} , response code : {}".format(api, response.status_code))
    except Exception as e:
        print("Failed to push data .............trying again")
        time.sleep(1)
        push_data_to_backend(trace_events, api=api)


def push_active_traces_data_to_backend_thread():
    while True:
        trace_events = []
        while not active_traces_q.empty():
            trace_event = active_traces_q.get().copy()
            trace_event.update({
                Constants.FIRST_TIMESTAMP: datetime.fromtimestamp(trace.get('first_timestamp')).isoformat() + "Z",
                Constants.LAST_TIMESTAMP: datetime.fromtimestamp(trace.get('last_timestamp')).isoformat() + "Z",
            })
            trace_events.append(trace_event)
        push_data_to_backend(trace_events, api=Constants.ACTIVE_TRACES_API_ENDPOINT)
        time.sleep(5)


def push_completed_traces_data_to_backend_thread():
    while True:
        completed_trace_events = []
        while not completed_traces_q.empty():
            trace_event = completed_traces_q.get().copy()
            trace_event.update({
                Constants.FIRST_TIMESTAMP: datetime.fromtimestamp(trace.get('first_timestamp')).isoformat() + "Z",
                Constants.LAST_TIMESTAMP: datetime.fromtimestamp(trace.get('last_timestamp')).isoformat() + "Z",
            })
            completed_trace_events.append(trace_event)
            push_data_to_backend(trace_event, api=Constants.COMPLETED_TRACES_API_ENDPOINT)
        # if completed_trace_events:
        #     push_data_to_backend(completed_trace_events, api=Constants.COMPLETED_TRACES_API_ENDPOINT)
        time.sleep(0.3)


if __name__ == '__main__':
    Logger(Constants.LOG_HANDLER_NAME)
    log.info("""tag tracker service started""")
    mqtt_client = init_mqtt(Constants.CENTRAL_MQTT_BROKER)

    t = threading.Thread(target=push_active_traces_data_to_backend_thread)
    t.start()
    t1 = threading.Thread(target=push_completed_traces_data_to_backend_thread)
    t1.start()

    while True:
        time.sleep(5)
        completed_traces = trace_tracker.get_completed_traces()
        active_traces = trace_tracker.get_active_traces()
        if len(completed_traces):
            log.info("*************************************************")
            for trace in completed_traces:
                central_topic = 'completed_traces'
                push_data_to_mqtt_broker(mqtt_client, json.dumps(trace), central_topic)
                completed_traces_q.put(trace)
        log.info("*******************Active Traces: {}**************".format(len(active_traces)))
        for trace in active_traces:
            central_topic = 'active_traces'
            log.info("trace details: {}".format(trace))
            push_data_to_mqtt_broker(mqtt_client, json.dumps(trace), central_topic)
            active_traces_q.put(trace)
        log.info("*********************************************")
        registered_devices = heart_beat_tracker.get_registered_devices()
        log.info("*******************Registered devices {} ************************".format(len(registered_devices)))
        log.info(registered_devices)
        log.info("*******************************************************")

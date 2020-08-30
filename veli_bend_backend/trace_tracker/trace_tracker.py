import logging
import threading
import time
from queue import Queue
from config import Constants

log = logging.getLogger(Constants.LOG_HANDLER_NAME)


class TraceTracker:
    def __init__(self, chk_timer=3, threshold=10):
        self.traces = {}
        self.threshold = threshold
        self.completed_traces_q = Queue()
        if chk_timer > 0:
            self.__set_interval(self.sanitize, chk_timer)

    def __set_interval(self, func, sec):
        """Thread based setInterval function - (not safe)"""

        def func_wrapper():
            self.__set_interval(func, sec)
            func()

        t = threading.Timer(sec, func_wrapper)
        t.start()
        return t

    def sanitize(self):
        """Remove expire tags"""
        now = int(time.time())
        for trace_uuid in list(self.traces):
            trace = self.traces[trace_uuid]
            if trace[Constants.LAST_TIMESTAMP] + self.threshold < now:
                log.info("==============trace completed ============ ")
                log.info("trace_uuid: {}".format(trace_uuid))
                log.info("trace details: {}".format(trace))
                del self.traces[trace_uuid]
                self.completed_traces_q.put(trace)
                log.info("========================================== ")

    def add(self, self_uuid, partner_uuid, covid_risk, distance, timestamp):
        trace_uuid = "{}@{}".format(self_uuid, partner_uuid)
        alternate_uuid = "{}@{}".format(partner_uuid, self_uuid)
        if trace_uuid not in self.traces and alternate_uuid not in self.traces:
            self.traces[trace_uuid] = {
                Constants.SELF_UUID: self_uuid,
                Constants.PARTNER_UUID: partner_uuid,
                Constants.COVID_RISK: covid_risk,
                Constants.MAX_DISTANCE: distance,
                Constants.MIN_DISTANCE: distance,
                Constants.FIRST_TIMESTAMP: timestamp,
                Constants.LAST_TIMESTAMP: timestamp,
            }
        else:
            trace_uuid = trace_uuid if trace_uuid in self.traces else alternate_uuid
            trace = self.traces[trace_uuid]
            trace.update({Constants.LAST_TIMESTAMP: int(time.time())})
            trace.update({Constants.MAX_DISTANCE: distance if distance > trace[Constants.MAX_DISTANCE] else trace[
                Constants.MAX_DISTANCE]})
            trace.update({Constants.MIN_DISTANCE: distance if distance < trace[Constants.MIN_DISTANCE] else trace[
                Constants.MIN_DISTANCE]})
            trace[Constants.DURATION] = trace[Constants.LAST_TIMESTAMP] - trace[Constants.FIRST_TIMESTAMP]
            self.traces[trace_uuid] = trace
        # log.debug(self.traces)

    def get_completed_traces(self):
        traces = []
        while not self.completed_traces_q.empty():
            trace = self.completed_traces_q.get()
            traces.append(trace)
        return traces

    def get_active_traces(self):
        traces = []
        for trace_uuid in list(self.traces):
            trace = self.traces[trace_uuid]
            traces.append(trace)
        return traces

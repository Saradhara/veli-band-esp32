import logging
import logging.handlers as handlers
import os
import time

from config import Constants


class Logger(object, ):
    def __init__(self,log_handler_name):
        const = Constants()
        #log_date_time = time.strftime("%b_%d_%Y@%H@%M@%S")
        name = log_handler_name
        logger = logging.getLogger(name)  # log_namespace can be replaced with your namespace
        logger.setLevel(const.LOG_LEVEL)
        if not logger.handlers:
            file_name = os.path.join(const.ROOT_DIR, const.LOGGING_DIR, '{}'.format(name))
            formatter = logging.Formatter(
                "[%(asctime)s] [%(threadName)s] %(levelname)s [%(filename)s:%(funcName)s:%(lineno)d] %(message)s")
            #handler = logging.FileHandler(file_name)
            handler = SizedTimedRotatingFileHandler(
                filename=file_name, maxBytes=64000*1024, backupCount=15,
                when='h', interval=24,
                # encoding='bz2',  # uncomment for bz2 compression
            )
            handler.setFormatter(formatter)
            handler.setLevel(logging.DEBUG)
            logger.addHandler(handler)

            console_handler = logging.StreamHandler()
            console_handler.setFormatter(formatter)
            logger.addHandler(console_handler)
        self._logger = logger

    def get(self):
        return self._logger


class SizedTimedRotatingFileHandler(handlers.TimedRotatingFileHandler):
    """
    Handler for logging to a set of files, which switches from one file
    to the next when the current file reaches a certain size, or at certain
    timed intervals
    """

    def __init__(self, filename, mode='a', maxBytes=0, backupCount=0, encoding=None,
                 delay=0, when='h', interval=1, utc=False):
        # If rotation/rollover is wanted, it doesn't make sense to use another
        # mode. If for example 'w' were specified, then if there were multiple
        # runs of the calling application, the logs from previous runs would be
        # lost if the 'w' is respected, because the log file would be truncated
        # on each run.
        if maxBytes > 0:
            mode = 'a'
        handlers.TimedRotatingFileHandler.__init__(
            self, filename, when, interval, backupCount, encoding, delay, utc)
        self.maxBytes = maxBytes

    def shouldRollover(self, record):
        """
        Determine if rollover should occur.

        Basically, see if the supplied record would cause the file to exceed
        the size limit we have.
        """
        if self.stream is None:  # delay was set...
            self.stream = self._open()
        if self.maxBytes > 0:  # are we rolling over?
            msg = "%s\n" % self.format(record)
            self.stream.seek(0, 2)  # due to non-posix-compliant Windows feature
            if self.stream.tell() + len(msg) >= self.maxBytes:
                return 1
        t = int(time.time())
        if t >= self.rolloverAt:
            return 1
        return 0


class MyCustomClass(object):
    def __init__(self):
        self.logger = Logger('event_detection_logger').get()  # accessing the "private" variables for each class

    def do_something(self):
        self.logger.info('Hello')

    def raise_error(self):
        self.logger.error('some error message')


if __name__ == '__main__':
    cl = MyCustomClass()
    logger = logging.getLogger('event_detection_logger')

    logger.info("vishnu")
    import logging
    try:
        1 / 0
    except Exception as e:
        logger.exception("message")


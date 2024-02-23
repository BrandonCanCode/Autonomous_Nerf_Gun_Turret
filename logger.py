# logger.py
# Sets log messages to the terminal and the a log file for each day.

import logging
import os
from datetime import datetime

def setup_logger():
    #Create logs directory if it doesn't exist
    os.makedirs("logs", exist_ok=True)

    #Define log file path with current date
    log_file = os.path.join("logs", f"{datetime.now().strftime('%Y_%m_%d')}.log")

    #Configure logging
    logging.basicConfig(
        level=logging.DEBUG,  #Set the minimum logging level to DEBUG
        format='[%(asctime)s %(levelname)s] %(message)s',
        handlers=[
            logging.FileHandler(log_file),
            logging.StreamHandler()
        ]
    )

    # Define a logger
    logger = logging.getLogger(__name__)

    return logger

# Usage example:
# from logger import setup_logger
# logger = setup_logger()
# logger.debug('This is a debug message')
# logger.info('This is an info message')
# logger.warning('This is a warning message')
# logger.error('This is an error message')
# logger.critical('This is a critical message')

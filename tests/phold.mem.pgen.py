#!python3
# -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
#
# Copyright (c) 2021 Lawrence Livermore National Laboratory
# All rights reserved.
#
# Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>


"""Generate parameter combinations for a PHOLD study"""

from maestrowf.datastructures.core import ParameterGenerator, StudyEnvironment
from maestrowf.datastructures.environment import Variable

import itertools as iter
import inspect
import logging


logger = None

def connect_to_logger():
    global logger
    if logger is None:
        ROOT_LOGGER = logging.getLogger(inspect.getmodule("maestrowf"))
        logger = logging.getLogger(__name__)


def LOG(args, level="info"):
    if level is "debug":
        logger.debug(args)
    else:
        logger.info(args)

def DEB(args):
    LOG(args, level="debug")

def get_custom_generator(env, **kwargs):
    """
    Configure a maestrowf.ParameterGenerator for a PHOLD study.

    :params env: A StudyEnvironment object containing custom information.
                 This is where we get fixed default values.
    :params kwargs: A dictionary of keyword arguments this function uses,
                 as supplied on the command line, typically.
    :returns: A ParameterGenerator populated with parameters.
    """

    connect_to_logger()
    
    # Get arguments, take from --parg "NAME:value" first, then
    # fall back to env vars from the yaml
    number_exp = int(kwargs.get("NUMBER_EXP", env.find("NUMBER_EXP").value))
    events_exp = int(kwargs.get("EVENTS_EXP", env.find("EVENTS_EXP").value))

    number = [ 2**j for j in range(1, number_exp) ]
    events = [ 2**j for j in range(7, 7 + events_exp)]
    LOG(f"Number exp: {number_exp}: {number}")
    LOG(f"Events exp: {events_exp}: {events}")

    number_values = []
    events_values = []
    runs = []

    for run, param_combo in enumerate(iter.product(number, events)):
        number_values.append(param_combo[0])
        events_values.append(param_combo[1])
        runs.append(run)

    params = {
        "RUN": {
            "values": runs,
            "label":  "R.%%"
        },
        "NUMBER": {
            "values": number_values,
            "label":  "N.%%"
        },
        "EVENTS": {
            "values": events_values,
            "label":  "E.%%"
        },
    }

    p_gen = ParameterGenerator()
    for key, value in params.items():
        p_gen.add_parameter(key, value["values"], value["label"])

    runs_list   = params['RUN']['values']
    number_list  = params['NUMBER']['values']
    events_list = params['EVENTS']['values']
    n_runs = len(runs_list)
    LOG(f"Number of runs: {n_runs}")

    DEB(f"Run\tNumber\tEvents")
    for i in range(0, n_runs):
        DEB(f"{runs_list[i]}\t{number_list[i]}\t{events_list[i]}")

    return p_gen


if __name__ == "__main__":

    # Logging setup, from maestro/conductor.py
    ROOTLOGGER = logging.getLogger(inspect.getmodule(__name__))
    ROOTLOGGER.setLevel(20)    # Enable Info and higher

    # Formatting of logger: just echo the messages
    LFORMAT = "%(message)s"
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter(LFORMAT))
    ROOTLOGGER.addHandler(handler)
    logger = logging.getLogger(__name__)
    
    study = StudyEnvironment()
    study.add(Variable('NUMBER_EXP', 3))
    study.add(Variable('EVENTS_EXP', 14))
    get_custom_generator(study)


 
    

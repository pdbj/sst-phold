#!/opt/local/bin/python3

import sst

import argparse
import os
import sys
import pdb


# PHOLD parameters
class PholdArgs(dict):
    def __init__(self):
        super().__init__()
        self.remote = 0.9
        self.minimum = 0.1
        self.average = 0.9
        self.number = 2
        self.verbose = False

    def __repr__(self):
        return f"remote: {self.remote}, " \
               f"min: {self.minimum}, " \
               f"avg: {self.average}, " \
               f"n: {self.number}, " \
               f"v: {self.verbose}"

    @property
    def validate(self):
        valid = True
        if self.number < 2:
            print(f"Invalid number: {self.number}, need at least 2")
            valid = False
        if self.events < 1:
            print(f"Invalid initial events: {self.events}, need at least 1")
            valid = False
        if self.remote < 0 or self.remote > 1:
            print(f"Invalid remote fraction: {self.remote}, must be in [0,1]")
            valid = False
        if self.minimum <= 0:
            print(f"Invalid minimum delay: {self.minimum}, must be > 0")
            valid = False
        if self.average < 0:
            print(f"Invalid average delay: {self.average}, must be >= 0")
            valid = False
        return valid


# Phold arguments instance
ph = PholdArgs()


def init_argparse() -> argparse.ArgumentParser:
    script = __file__
    parser = argparse.ArgumentParser(
        usage=f"sst {script} [OPTION]...",
        description="Execute the standard PHOLD benchmark.")
    parser.add_argument(
        '-r', '--remote', action='store', type=float,
        help=f"Fraction of events which should be scheduled for other LPs")
    parser.add_argument(
        '-m', '--minimum', action='store', type=float,
        help=f"Minimum inter-event delay, in seconds. "
             f"Must be greater than zero.")
    parser.add_argument(
        '-a', '--average', action='store', type=float,
        help=f"Average additional inter-event delay, in seconds. "
             f"This will be added to the min delay.")
    parser.add_argument(
        '-n', '--number', action='store', type=int,
        help=f"Total number of LPs. Must be at least 2.")
    parser.add_argument(
        '-v', '--verbose', action='store_true',
        help=f"Verbose output")
    return parser


def parse():
    parser = init_argparse()
    parser.parse_args(namespace=ph)
    if not ph.validate:
        parser.print_help()
        sys.exit(1)


parse()
print(f"Expect {ph}")

obj = sst.Component("phold-1",
                    "phold.Phold")
obj.addParams(vars(ph)) # pass as simple dictionary

# Add links
# Set link latency to minimum?

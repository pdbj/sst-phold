#!python3

import sst

import argparse
import os
import sys

ME = "Phold.py: "

def phPrint (args):
    print(ME, args)

# PHOLD parameters
class PholdArgs(dict):
    def __init__(self):
        super().__init__()
        self.remote = 0.9
        self.minimum = 1
        self.average = 10
        self.stop = 10
        self.number = 2
        self.events = 1
        self.pverbose = False

    def __str__(self):
        return f"remote: {self.remote}, " \
               f"min: {self.minimum}, " \
               f"avg: {self.average}, " \
               f"st: {self.stop}, " \
               f"n: {self.number}, " \
               f"ev: {self.events}, " \
               f"v: {self.pverbose}, " \

    @property
    def validate(self):
        valid = True
        if self.remote < 0 or self.remote > 1:
            phPrint(f"Invalid remote fraction: {self.remote}, must be in [0,1]")
            valid = False
        if self.minimum <= 0:
            phPrint(f"Invalid minimum delay: {self.minimum}, must be > 0")
            valid = False
        if self.average < 0:
            phPrint(f"Invalid average delay: {self.average}, must be >= 0")
            valid = False
        if self.stop < 0:
            phPrint(f"Invalid stop time: {self.stop}, must be > 0")
        if self.number < 2:
            phPrint(f"Invalid number: {self.number}, need at least 2")
            valid = False
        if self.events < 1:
            phPrint(f"Invalid initial events: {self.events}, need at least 1")
            valid = False
        return valid


    def init_argparse(self) -> argparse.ArgumentParser:
        script = os.path.basename(__file__)
        parser = argparse.ArgumentParser(
            usage=f"sst {script} [OPTION]...",
            description="Execute the standard PHOLD benchmark.")
        parser.add_argument(
            '-r', '--remote', action='store', type=float,
            help=f"Fraction of events which should be scheduled for other LPs, in [0,1].")
        parser.add_argument(
            '-m', '--minimum', action='store', type=float,
            help=f"Minimum inter-event delay, in seconds.  Must be >0.")
        parser.add_argument(
            '-a', '--average', action='store', type=float,
            help=f"Average additional inter-event delay, in seconds. "
             f"This will be added to the min delay, and must be >= 0")
        parser.add_argument(
            '-s', '--stop', action='store', type=float,
            help=f"Total simulation time, in seconds. Must be > 0.")
        parser.add_argument(
            '-n', '--number', action='store', type=int,
            help=f"Total number of LPs. Must be at least 2.")
        parser.add_argument(
            '-e', '--events', action='store', type=int,
            help=f"Number of initial events per LP. Must be >= 1.")
        parser.add_argument(
            # '--verbose' conflicts with SST, even after --
            '-v', '--pverbose', action='store_true',
            help=f"Verbose output")
        return parser

    def parse(self):
        parser = self.init_argparse()
        parser.parse_args(namespace=self)
        if not self.validate:
            parser.print_help()
            sys.exit(1)
        if ph.pverbose:
            phPrint(f"  expect {ph}")


# -- Main --

print(f"\n")
phPrint(f"Creating PHOLD Benchmark")

# Phold arguments instance
ph = PholdArgs()
ph.parse()


# Create the LPs
phPrint(f"Creating {ph.number} LPs")
lps = []
for i in range(ph.number):
    if ph.pverbose:
        phPrint(f"  Creating LP {i}")
    lp = sst.Component(str(i), "phold.Phold")
    lp.addParams(vars(ph))  # pass ph as simple dictionary
    lps.append(lp)

# Add links
phPrint(f"Creating complete graph")
# min latency
latency = str(ph.minimum) + "s"
for i in range(ph.number):
    for j in range(i + 1, ph.number):
        if ph.pverbose:
            phPrint(f"  Creating link {i}_{j}")
        link = sst.Link(str(i) + "_" + str(j))
        if ph.pverbose:
            phPrint(f"    creating tuples")
        # links cross connect ports: 
        # port number gives the LP id on the other side of the link
        li = lps[i], "port_" + str(j), latency
        lj = lps[j], "port_" + str(i), latency
        if ph.pverbose:
            phPrint(f"      {li}")
            phPrint(f"      {lj}")
            phPrint(f"    connecting {i} to {j}")
        link.connect(li, lj)

phPrint(f"Done")


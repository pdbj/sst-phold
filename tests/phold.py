#!python3

# import sst is managed below

import argparse
import math
import os
import sys

ME = "Phold.py: "


def phprint(args):
    print(ME, args)

def vprint(l, args):
    if ph.pyVerbose >= l: phprint(args)

# Print dots to show progress
class Dot():
    def __init__(self, N):
        # limit dots to a single 80 char line
        self.nDots = math.ceil(N / 79.0)
        self.dotCount = 0
        self.dotted = False

    def dot(self, level, show = True):
        if show: self.dotCount += 1
        if ph.pyVerbose < level:
            if self.dotCount >= self.nDots :
                if show: 
                    print('.', end='', flush=True)
                    self.dotted = True
                self.dotCount = 0
            return False
        else:
            return True
        
    def done(self):
        if self.dotted: print("")

        
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
        self.delays = False
        self.pverbose = 0
        self.pyVerbose = 0

    def __str__(self):
        return f"remote: {self.remote}, " \
               f"min: {self.minimum}, " \
               f"avg: {self.average}, " \
               f"stop: {self.stop}, " \
               f"nodes: {self.number}, " \
               f"events: {self.events}, " \
               f"delays: {self.delays}, " \
               f"verbose: {self.pverbose}, " \
               f"pyVerbose: {self.pyVerbose}"

    def print(self):
        print(f"    Remote LP fraction:                   {self.remote}")
        print(f"    Minimum inter-event delay:            {self.minimum} s")
        print(f"    Additional exponential average delay: {self.average} s")
        print(f"    Stop time:                            {self.stop} s")
        print(f"    Number of LPs:                        {self.number}")
        print(f"    Number of initial events per LP:      {self.events}")
        print(f"    Output delay histogram:               {self.delays}")
        print(f"    Verbosity level:                      {self.pverbose}")
        print(f"    Python script verbosity level:        {self.pyVerbose}")

    @property
    def validate(self):
        valid = True
        if self.remote < 0 or self.remote > 1:
            phprint(f"Invalid remote fraction: {self.remote}, must be in [0,1]")
            valid = False
        if self.minimum <= 0:
            phprint(f"Invalid minimum delay: {self.minimum}, must be > 0")
            valid = False
        if self.average < 0:
            phprint(f"Invalid average delay: {self.average}, must be >= 0")
            valid = False
        if self.stop < 0:
            phprint(f"Invalid stop time: {self.stop}, must be > 0")

        self.number = int(self.number)
        if self.number < 2:
            phprint(f"Invalid number: {self.number}, need at least 2")
            valid = False

        self.events = int(self.events)
        if self.events < 1:
            phprint(f"Invalid initial events: {self.events}, need at least 1")
            valid = False
        return valid

    def init_argparse(self) -> argparse.ArgumentParser:

        script = os.path.basename(__file__)
        parser = argparse.ArgumentParser(
            usage=f"sst {script} [OPTION]...",
            description="Execute the standard PHOLD benchmark.")
        parser.add_argument(
            '-r', '--remote', action='store', type=float,
            help=f"Fraction of events which should be scheduled for other LPs. "
            f"Must be in [0,1], default {self.remote}.")
        parser.add_argument(
            '-m', '--minimum', action='store', type=float,
            help=f"Minimum inter-event delay, in seconds. "
            f"Must be >0, default {self.minimum}.")
        parser.add_argument(
            '-a', '--average', action='store', type=float,
            help=f"Average additional inter-event delay, in seconds. "
             f"This will be added to the min delay, and must be >= 0, "
            f"default {self.average}.")
        parser.add_argument(
            '-s', '--stop', action='store', type=float,
            help=f"Total simulation time, in seconds. "
            f"Must be > 0, default {self.stop}.")
        parser.add_argument(
            '-n', '--number', action='store', type=float,
            help=f"Total number of LPs. "
            f"Must be > , default {self.number}.")
        parser.add_argument(
            '-e', '--events', action='store', type=float,
            help=f"Number of initial events per LP. "
            f"Must be > 0, default {self.events}")
        parser.add_argument(
            # '--verbose' conflicts with SST, even after --
            '-v', '--pverbose', action='count',
            help=f"Phold module verbosity, default {self.pverbose}.")
        parser.add_argument(
            '-d', '--delays', action='store_true',
            help=f"Whether to output delay histogram, default {self.delays}.")
        parser.add_argument(
            '-V', '--pyVerbose', action='count',
            help=f"Python script verbosity, default {self.pyVerbose}.")
        return parser

    def parse(self):
        parser = self.init_argparse()
        parser.parse_args(namespace=self)
        if not self.validate:
            parser.print_help()
            sys.exit(1)
            
        phprint(f"Configuration:")
        self.print()


# -- Main --
print(f"\n")
phprint(f"Creating PHOLD Benchmark")

# If sst not found, we're just debugging this script
try:
    import sst
    just_script = False
    phprint(f"Importing SST module")
except:
    just_script = True
    phprint(f"No SST module, just debugging this script")


# Phold arguments instance
ph = PholdArgs()
ph.parse()

if just_script:
    sys.exit(1)

# Create the LPs
phprint(f"Creating {ph.number} LPs")
dotter = Dot(ph.number)
lps = []
for i in range(ph.number):
    if dotter.dot(1): vprint(1, f"  Creating LP {i}")
    lp = sst.Component(str(i), "phold.Phold")
    lp.addParams(vars(ph))  # pass ph as simple dictionary
    lps.append(lp)
dotter.done()

# min latency
latency = str(ph.minimum) + "s"

# Add links
nLinks = int(ph.number * (ph.number - 1) / 2)
phprint(f"Creating complete graph with latency {latency} ({nLinks} total)")
dotter = Dot(nLinks)
for i in range(ph.number):
    for j in range(i + 1, ph.number):

        link = sst.Link(str(i) + "_" + str(j))

        if dotter.dot(2, False): vprint(2, f"  Creating link {i}_{j}")
        # links cross connect ports: 
        # port number gives the LP id on the other side of the link
        li = lps[i], "port_" + str(j), latency
        lj = lps[j], "port_" + str(i), latency

        if dotter.dot(3):
            vprint(3, f"    creating tuples")
            vprint(3, f"      {li}")
            vprint(3, f"      {lj}")
            vprint(3, f"    connecting {i} to {j}")

        link.connect(li, lj)
dotter.done()

# Enable statistics
statLevel = 1 + ph.delays
phprint(f"Enabling statistics at level {statLevel}")
sst.setStatisticLoadLevel(statLevel)
sst.setStatisticOutput("sst.statOutputConsole")

# Always enable Count, report only at end
# Stat type accumulator is the default
sst.enableStatisticsForComponentType("phold.Phold", ["Count"],
                                     {"rate" : "0ns"})

if ph.delays:
    sst.enableStatisticsForComponentType("phold.Phold", ["Delays"],
                                         {"type" : "sst.HistogramStatistic",
                                          "rate" : "0ns",
                                          "minvalue" : "0",
                                          "binwidth" : "1",
                                          "numbins" : "50" } )

phprint(f"Done\n")

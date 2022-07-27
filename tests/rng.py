#!python3
# -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
#
# Copyright (c) 2021 Lawrence Livermore National Laboratory
# All rights reserved.
#
# Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>

"""
Test the SST RNG performance with a simple component.
"""

# import sst is managed below

import argparse
import math
import os
import sys

phold_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, phold_dir)
import progress_dot as dot


def meprint(args):
    """Print messages prefixed by our script name."""
    script = os.path.basename(__file__)
    print(script, ": ", args)

def vprint(level, args):
    """Print a verbose message at verbosity level."""
    if rng.pyVerbose >= level:
        meprint(args)


# RNG parameters
class RngArgs(dict):
    """Argument processing and validation.

    Methods
    -------
    parse() : None
        Parse the arguments and store the values.

    print() : None
        Pretty print the configuration.

    Attributes
    ----------
    See print() or _init_argparse(), or run with '--help'
    """
    def __init__(self):
        super().__init__()
        self.number = 1
        self.samples = 100
        self.pverbose = 0
        self.pyVerbose = 0

    def __str__(self):
        return f"comp: {self.number}, " \
               f"samples: {self.samples}, " \
               f"verbose: {self.pverbose}, " \
               f"pyVerbose: {self.pyVerbose}"

    def print(self):
        """Pretty print the configuration."""
        print(f"    Number of components:           {self.number}")
        print(f"    Number of samples:              {self.samples}")
        print(f"    Verbosity level:                {self.pverbose}")
        print(f"    Python script verbosity level:  {self.pyVerbose}")

    @property
    def validate(self):
        """Validate the configuration."""
        valid = True
        self.number = int(self.number)
        if self.number < 1:
            meprint(f"Invalid number of components: {self.number}, need at least 1")
            valid = False

        self.samples = int(self.samples)
        if self.samples < 1:
            meprint(f"Invalid number of samples: {self.samples}, need at least 1")
            valid = False
        return valid

    def init_argparse(self) -> argparse.ArgumentParser:
        """Configure the argument parser with our arguments."""
        script = os.path.basename(__file__)
        parser = argparse.ArgumentParser(
            usage=f"sst {script} [OPTION]...",
            description="Execute an RNG timing benchmark.")
        parser.add_argument(
            '-n', '--number', action='store', type=float,
            help=f"Total number of components. "
            f"Must be > 0, default {self.number}.")
        parser.add_argument(
            '-s', '--samples', action='store', type=float,
            help=f"Number of samples per component. "
            f"Must be > 1, default {self.samples}")
        parser.add_argument(
            # '--verbose' conflicts with SST, even after --
            '-v', '--pverbose', action='count',
            help=f"Rng module verbosity, default {self.pverbose}.")
        parser.add_argument(
            '-V', '--pyVerbose', action='count',
            help=f"Python script verbosity, default {self.pyVerbose}.")
        return parser

    def parse(self):
        """Parse and validate the script arguments.

        If the configuration is not valid print the help and exit.

        If pyVerbose is on show the configuration.
        """
        parser = self.init_argparse()
        parser.parse_args(namespace=self)
        if not self.validate:
            parser.print_help()
            sys.exit(1)

        meprint(f"Configuration:")
        self.print()


# -- Main --
print(f"\n")
meprint(f"Creating RNG timing benchmark")

# If sst not found, we are just debugging this script
try:
    import sst
    just_script = False
    meprint(f"Importing SST module")
except ImportError as error:
    just_script = True
    meprint(f"No SST module, just debugging this script")


# Rng arguments instance
rng = RngArgs()
rng.parse()

if just_script:
    sys.exit(1)

# Create the LPs
meprint(f"Creating {rng.number} components")
dotter = dot.Dot(rng.number, rng.pyVerbose)
lps = []
for i in range(rng.number):
    if dotter.dot(1):
        vprint(1, f"  Creating LP {i}")
    lp = sst.Component("rng_" + str(i), "phold.Rng")
    lp.addParams(vars(rng))  # pass rng params as simple dictionary
    lps.append(lp)
dotter.done()

# Nuisance links
latency = "1 ms"
meprint(f"Creating complete ring with latency {latency} ({rng.number} total links)")
dotter = dot.Dot(rng.number, rng.pyVerbose)
for i in range(rng.number):
    j = (i + 1) % rng.number
    link = sst.Link("rng_" + str(i) + "_" + str(j))
    if dotter.dot(2, False):
        vprint(2, f"  Creating link rng_{i}_{j}")
    li = lps[i], 'portL', latency
    lj = lps[j], 'portR', latency
    if (dotter.dot(3)):
        vprint(3, f"    creating tuple")
        vprint(3, f"      {li}")
        vprint(3, f"      {lj}")
        vprint(3, f"    connecting {i} to {j}")
    link.connect(li, lj)
dotter.done()

# Enable statistics
stat_level = 1
meprint(f"Enabling statistics at level {stat_level}")
sst.setStatisticLoadLevel(stat_level)
sst.setStatisticOutput("sst.statOutputConsole")

# Always enable Count, report only at end
# Stat type accumulator is the default

meprint(f"Done\n")

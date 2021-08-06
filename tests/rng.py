#!python3

# import sst is managed below

import argparse
import math
import os
import sys

ME = "rng.py: "


def meprint(args):
    print(ME, args)

def vprint(l, args):
    if rng.pyVerbose >= l: meprint(args)

# Print dots to show progress
class Dot():
    def __init__(self, N):
        # limit dots to a single 80 char line
        self.nDots = math.ceil(N / 79.0)
        self.dotCount = 0
        self.dotted = False

    def dot(self, level, show = True):
        if show: self.dotCount += 1
        if rng.pyVerbose < level:
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

        
# RNG parameters
class RngArgs(dict):
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
        print(f"    Number of components:           {self.number}")
        print(f"    Number of samples:              {self.samples}")
        print(f"    Verbosity level:                {self.pverbose}")
        print(f"    Python script verbosity level:  {self.pyVerbose}")

    @property
    def validate(self):
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

# If sst not found, we're just debugging this script
try:
    import sst
    just_script = False
    meprint(f"Importing SST module")
except:
    just_script = True
    meprint(f"No SST module, just debugging this script")


# Rng arguments instance
rng = RngArgs()
rng.parse()

if just_script:
    sys.exit(1)

# Create the LPs
meprint(f"Creating {rng.number} components")
dotter = Dot(rng.number)
lps = []
for i in range(rng.number):
    if dotter.dot(1): vprint(1, f"  Creating LP {i}")
    lp = sst.Component(str(i), "phold.Rng")
    lp.addParams(vars(rng))  # pass rng params as simple dictionary
    lps.append(lp)
dotter.done()

# No links

# Enable statistics
statLevel = 1
meprint(f"Enabling statistics at level {statLevel}")
sst.setStatisticLoadLevel(statLevel)
sst.setStatisticOutput("sst.statOutputConsole")

# Always enable Count, report only at end
# Stat type accumulator is the default

meprint(f"Done\n")

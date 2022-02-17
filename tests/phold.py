#! python3
"""
Configuration for the classic PHOLD benchmark.
"""

# import sst is managed below

import argparse
import os
import pprint
import sys
import textwrap

phold_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, phold_dir)
import progress_dot as dot


def phprint(args):
    """Print progress information, prefixed by our name."""
    script = os.path.basename(__file__)
    print(script, ': ', args)

    # print(script, ': ', end='')
    # for arg in args:
    #     print(arg, sep='', end='')
    # print('')

def vprint(level: int, args):
    """Print a verbose message at verbosity level."""
    if phold.pyVerbose >= level:
        phprint(args)

def dprint(message: str, dictionary: dict):
    """Print a header message, followed by a pretty printed dictionary d."""
    if phold.pyVerbose:
        phprint(message)
        dict_pretty = pprint.pformat(dictionary, indent=2, width=1)
        print(textwrap.indent(dict_pretty, '    '))



# PHOLD parameters
class PholdArgs(dict):
    """PHOLD argument processing and validation.

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

    # pylint: disable=too-many-instance-attributes

    def __init__(self):
        """Initialize the configuration with default values.

        Attributes
        ----------
        See print() or _init_argparse(), or run with '--help'
        """
        super().__init__()
        self.remote = 0.9
        self.minimum = 1
        self.average = 9
        self.stop = 10
        self.number = 2
        self.events = 1
        self.delays = False
        self.pverbose = 0
        self.pyVerbose = 0
        self.TIMEBASE = 's'

    def __str__(self) -> str:
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
        """Pretty print the configuration."""

        # Syncrhonization duty factor: window size divided by total expected delay
        duty_factor = self.minimum / (self.minimum + self.average)
        # Expected number of events at each LP per window
        ev_per_win = self.events * duty_factor
        # Min target number of events per window
        min_ev_per_win = 10
        # Suggested minimum number of events per LP
        min_events = round(min_ev_per_win / duty_factor)

        print(f"    Remote LP fraction:                   {self.remote}")
        print(f"    Minimum inter-event delay:            {self.minimum} {self.TIMEBASE}")
        print(f"    Additional exponential average delay: {self.average} {self.TIMEBASE}")
        print(f"    Stop time:                            {self.stop} {self.TIMEBASE}")
        print(f"    Number of LPs:                        {self.number}")
        print(f"    Number of initial events per LP:      {self.events}")

        print(f"    Average events per window:            {ev_per_win:.2f}")
        if ev_per_win < min_ev_per_win:
            print(f"      (Too low!  Suggest setting '--events={min_events}')")

        print(f"    Output delay histogram:               {self.delays}")
        print(f"    Verbosity level:                      {self.pverbose}")
        print(f"    Python script verbosity level:        {self.pyVerbose}")

    @property
    def _validate(self) -> bool:
        """Validate the configuration."""
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

    def _init_argparse(self) -> argparse.ArgumentParser:
        """Configure the argument parser with our arguments."""
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
            help=f"Minimum inter-event delay, in {self.TIMEBASE}. "
            f"Must be >0, default {self.minimum}.")
        parser.add_argument(
            '-a', '--average', action='store', type=float,
            help=f"Average additional inter-event delay, in {self.TIMEBASE}. "
            f"This will be added to the min delay, and must be >= 0, "
            f"default {self.average}.")
        parser.add_argument(
            '-s', '--stop', action='store', type=float,
            help=f"Total simulation time, in {self.TIMEBASE}. "
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
        """Parse and validate the script arguments.

        If the configuration is not valid print the help and exit.

        If pyVerbose is on show the configuration. (The C++ Phold class
        will always print the final configuration.)
        """
        parser = self._init_argparse()
        parser.parse_args(namespace=self)
        if not self._validate:
            parser.print_help()
            sys.exit(1)

        if self.pyVerbose:
            phprint(f"Configuration:")
            self.print()


# -- Main --

print(f"")
phprint(f"Creating PHOLD Benchmark")

# If sst not found, we're just debugging this script
try:
    import sst
    just_script = False
    phprint(f"Importing SST module")
except ImportError as error:
    just_script = True
    phprint(f"No SST module, just debugging this script")


# Phold arguments instance
phold = PholdArgs()
phold.parse()

if just_script:
    sys.exit(1)

# Create the LPs
phprint(f"Creating {phold.number} LPs")
dotter = dot.Dot(phold.number, phold.pyVerbose)
lps = []
for i in range(phold.number):
    if dotter.dot(1):
        vprint(1, f"  Creating LP {i}")
    lp = sst.Component(str(i), 'phold.Phold')
    lp.addParams(vars(phold))  # pass ph as simple dictionary
    lps.append(lp)
dotter.done()

# min latency
latency = str(phold.minimum) + ' ' + phold.TIMEBASE

# Add links
num_links = int(phold.number * (phold.number - 1) / 2)
phprint(f"Creating complete graph with latency {latency} ({num_links} total)")
dotter = dot.Dot(num_links, phold.pyVerbose)
for i in range(phold.number):
    for j in range(i + 1, phold.number):

        link = sst.Link(str(i) + '_' + str(j))

        if dotter.dot(2, False):
            vprint(2, f"  Creating link {i}_{j}")
        # links cross connect ports:
        # port number gives the LP id on the other side of the link
        li = lps[i], 'port_' + str(j), latency
        lj = lps[j], 'port_' + str(i), latency

        if dotter.dot(3):
            vprint(3, f"    creating tuples")
            vprint(3, f"      {li}")
            vprint(3, f"      {lj}")
            vprint(3, f"    connecting {i} to {j}")

        link.connect(li, lj)
dotter.done()

# Enable statistics
stat_level = 1 + phold.delays
phprint(f"Enabling statistics at level {stat_level}")
sst.setStatisticLoadLevel(stat_level)
sst.setStatisticOutput('sst.statOutputConsole')

# Common stats configuration:
# rate: 0ns:    Only report results at end
# stopat:       Stop collecting at stop time
stats_config = {'rate': '0ns',
                'stopat' : str(phold.stop) + phold.TIMEBASE}

# We always enable the send/recv counters
# Stat type accumulator is the default, so don't need state it explicitly
dprint("  Accumulator config:", stats_config)

sst.enableStatisticsForComponentType('phold.Phold', ['SendCount'], stats_config)
sst.enableStatisticsForComponentType('phold.Phold', ['RecvCount'], stats_config)

if phold.delays:
    delay_mean = phold.minimum + phold.average
    numbins = 50
    binwidth = round(5 * delay_mean / numbins)
    delays_config = {**stats_config,
                     **{'type' : 'sst.HistogramStatistic',
                        'minvalue' : '0',
                        'binwidth' : str(binwidth),
                        'numbins'  : str(numbins)}}

    dprint("Delay histogram config:", delays_config)

    sst.enableStatisticsForComponentType('phold.Phold', ['Delays'], delays_config)

phprint(f"Done\n")

#!/bin/python3
# -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
#
# Copyright (c) 2021 Lawrence Livermore National Laboratory
# All rights reserved.
#
# Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>


import math

class Dot():
    """Print a series of dots to show progress.

    The class is initialized with the expected number of items.
    The processing of each item is logged by calling dot().
    This will print a dot '.' for each item, or periodically if there are
    more than 80 items.

    Whether a dot is printed

    Parameters
    ----------
    N : int
        The total number of items expected.
    level : int
        Threshold level above which printing should be suppressed
        (presumably because the caller has a more verbose progess indication).

    Methods
    -------
    dot(level: int, show=True) -> bool
        Log the processing of an item at level.  Suppress
    done()
        If any dots were printed add a newline.

    """

    def __init__(self, N: int, level: int):
        """
        Parameter
        ----------
        N : int
            The total number of items expected.
        level : int
            Threshold level at which to suppress printing.

        Attributes
        ----------
        _dots_per : int
            Number of items per dot.
        _level : int
            Threshold level at which to suppress printing.
        _item_count : int
            How many items have been logged since the last dot was printed.
        _dotted : bool
            True if we've printed any dots, so we add a newline when done.
        """
        # limit dots to a single 80 char line
        self._dots_per = math.ceil(N / 79.0)
        self._level = level
        self._item_count = 0
        self._dotted = False

    def dot(self, level: int, show=True) -> bool:
        """Log the processing of an item.

        Parameters
        ----------
        level : int
            Verbosity level of this call. If this is greater than self.level
            printing is suppressed.
        show : bool
            Alternate method to suppress printing

        Returns True if level was too high, allowing this pattern:
           if dotter.dot(l, False): print("Alternate verbose message")

        """
        if show:
            self._item_count += 1
        if self._level < level:
            if self._item_count >= self._dots_per:
                if show:
                    print('.', end='', flush=True)
                    self._dotted = True
                self._item_count = 0
            return False

        return True

    def done(self):
        """Finish by adding a newline, if any dots were printed."""
        if self._dotted:
            print("")


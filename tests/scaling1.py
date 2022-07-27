#!python3
# -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
#
# Copyright (c) 2021 Lawrence Livermore National Laboratory
# All rights reserved.
#
# Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>

"""
Generate a list of interesting PHOLD test points.
"""

from operator import mul

nodes = [2, 5, 10]
events = [1, 5, 10]
means = [10, 5, 2]
stops = [30, 100, 500]

ncases = list(map(len, [nodes, events, means, stops]))
print(f"\nTotal cases: {str(ncases)}")
ncases = map(mul, ncases)
print(f"\nTotal cases: {list(ncases)}")
exit(0)

# from tabulate import tabulate
# print(tabulate([['Alice', 24],
#                 ['Bob', 19]],    headers=['Name', 'Age']))

titles = ["Nodes", "Events", "Mean", "Stop", "Time"]

cases = []
for n in nodes:
    for e in events:
        for m in means:
            for s in stops:
                time = n * e * s / m
                cases += [n, e, m, s, time]

print(tabulate(cases, headers=titles))

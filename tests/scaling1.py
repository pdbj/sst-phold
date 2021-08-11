#!python3
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

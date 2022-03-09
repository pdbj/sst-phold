#!/bin/bash

srun -N 1 -n 2 sst  --print-timing-info tests/phold.py -- -s 100 -n 1024 -e 128


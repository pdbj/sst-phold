#!/bin/bash

#SBATCH --nodes=2
#SBATCH --ntasks-per-node=2
#SBATCH --job-name=phold
#SBATCH --partition=pdebug
#SBATCH --account=wireless

date
pwd
which sst

srun --nodes=2 --ntasks=2 sst --num_threads=2 --print-timing-info tests/phold.py -- --number 512 --events 1024 --stop 1000



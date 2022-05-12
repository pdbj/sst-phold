#!/bin/bash

#SBATCH --nodes=1
#SBATCH --ntasks-per-node=2
#SBATCH --job-name=phold
#SBATCH --partition=pdebug
#SBATCH --account=dance

date
echo "PWD:" `pwd`
echo "SST:" `which sst`

srun --nodes=1 --ntasks=2 \
     sst --num_threads=2 \
     tests/phold.py -- \
     --number 512 --events 1024 --stop 1000 --buffer 1000 --thread 0.5

# --number 512 --events 1024 --stop 1000
# --number 4 --events 1 --stop 100 --pverbose --buffer 1000

echo
date

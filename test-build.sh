#!/bin/sh

# Test SST at a particular named commit



pushd ../sst-core
make clean
git checkout $1
git reset --hard
./autogen.sh
CXX=mpicxx CXXFLAGS="-std=c++17 -g -Og -faligned-new" \
  ./configure --prefix=$PWD/../build/test
make -j install
popd

make -j debug install

srun -A wireless -p pdebug --nodes=1 --ntasks-per-node=4 \
  sst --num_threads=4 \
  tests/phold.py -- -s 1000 -n 1024 -e 128

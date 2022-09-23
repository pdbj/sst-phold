#!/bin/bash

# Set bash DEBUG and EXIT traps to report command failures
function set_traps
{
    # exit when any command fails
    set -e

    # keep track of the last executed command
    trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
    # echo an error message before exiting
    trap 'echo "\"${last_command}\" command failed with exit code $?."' EXIT
}

# Unset bash traps
function reset_traps
{
    trap - DEBUG EXIT
    set +e
}
    

# pushd (change) directory and log it, optionally log to file
function push_cwd # dir logfile
{
    command pushd $1 >/dev/null
    if [ $# -gt 1 ] ; then
	echo PWD=$PWD  2>&1 | tee -a $2
    else
	echo PWD=$PWD
    fi
}

# popd directory (return to previous), and optionally log to file
function pop_cwd # logfile
{
    command popd >/dev/null
    if [ $# -gt 1 ] ; then
	echo PWD=$PWD  2>&1 | tee -a $1
    else
	echo PWD=$PWD
    fi
}

# Set the build/sst symbolic link to point to 
# a particular SST installed version.
function set_sst # install-dir
{
    echo
    echo "Switching to SST version $1"
    echo "****************************************"
    push_cwd ~/Code/SST/build
    rm -f sst
    ln -fsT $1 sst
    echo "which sst: " `which sst`
    echo "ls -l sst: " `ls -ld sst`

    pop_cwd
}


function build # install commit config-args
{
    local install=$1 ; shift
    local commit=$1  ; shift
    local logf=~/Code/SST/phold/profile/build.${install}.log
    echo "Logging build to $logf"

    echo | tee $logf
    echo "Building $install from $commit" | tee -a $logf
    echo "******************************" | tee -a $logf
    date  | tee -a $logf
    push_cwd ~/Code/SST/sst-core $logf

    local oldPS4=$PS4
    PS4="--- build: "
    set -x

    make clean  2>&1 | tee -a $logf
    git checkout $commit 2>&1 | tee -a $logf
    git reset --hard 2>&1 | tee -a $logf
    ./autogen.sh 2>&1 | tee -a $logf
    CXX=mpicxx CXXFLAGS="-std=c++17 -g -O3 -faligned-new" \
       ./configure --prefix=$PWD/../build/$install $* \
        2>&1 | tee -a $logf
    make -j install  2>&1 | tee -a $logf

    set +x
    PS4=oldPS4

    pop_cwd $logf

    set_sst $install
    make -j opt install 2>&1 | tee -a $logf

    date  | tee -a $logf
}

function save_perf # label
{
    echo "Checking for old profiling output files"
    ls rank_*thread_* 1>/dev/null 2>/dev/null || return 0 
    
    for f in rank_*thread_* ; do 
	newf="profile/${label}.$f"
	echo "mv $f	-->	$newf"
	mv $f $newf
    done
}

phold_args="--stop 2000 --number 2048 --events 128"
#phold_args="--stop 100 --number 248 --events 128"

function run_one # label sst-args
{
    local label=$1;  shift
    local logf=profile/${label}.run.log
    echo
    echo "Running $label"
    echo "******************************"
    echo "   with args: $*"
    echo "   to $logf"
    sst $* tests/phold.py -- $phold_args \
	2>& 1 | tee $logf
    save_perf $label
}

function run_srun # label sst-args
{
    local label=${1};  shift
    local logf=profile/${label}.run.log
    echo
    echo "Srunning $label"
    echo "******************************"
    echo "   with args: $*"
    echo "   to $logf"
    srun --nodes 4 --ntasks-per-node=1 \
	 sst $* tests/phold.py -- $phold_args \
	2>& 1 | tee $logf
    save_perf $label
}

function run_suite # version perf-on/off.iter sst-args
{
    local version=$1;  shift
    perf=$1;     shift
    set_sst $version

    run_one  ${version}.${perf}.serial $*
    run_one  ${version}.${perf}.thread4 --num_threads=4 $*

    if [ "${SLURM_JOB_ID:-}" != "" ]; then
	run_srun ${version}.${perf}.mpi4 $*
	run_srun ${version}.${perf}.mpi4.thread4 --num_threads=4 $*
    else
	echo "===  SKIPPING MPI RUN ${version}.${perf}.mpi ==="
	echo "===  SKIPPING MPI RUN ${version}.${perf}.mpi.thread4 ==="
    fi
}

function run_all # iteration
{
    local iter=$1;   shift

    run_suite perf-old-no  off.${iter}
    run_suite perf-old-yes on.${iter}
    run_suite perf-new     off.${iter}
    run_suite perf-new     on.${iter}  --enable-profiling="event;clock;sync"
}

##################################################

if [ "${1:-}" == "-h" ] ; then
    cat <<-EOF
	Usage: $0 [build | run | batch]
	
	build:  Build old profiling version, with profiling off or on,
	        and new profiling.
	
	run:    Run the non-mpi cases
	
	batch:  Run all cases, including the MPI runs
	EOF
    exit 1
fi


set_traps

# Use 'batch' arg on the command line to submit as a batch job
if [ "${1:-}" == "batch" ] ; then

    cd ~/Code/SST/phold >/dev/null
    sbatch \
	--account=autonomy \
	--partition=pbatch \
	--job-name=sst-profile \
	--output=profile/profile-%j.log \
	--nodes=4 \
	--time=6:00:00 \
	profile/profile-timing.sh

    cd -  >/dev/null
    reset_traps

    # wait for the job to start
    while squeue -u barnes | grep -q "Priority\|Resources" ; do
	sleep 2
    done
    squeue -u barnes

    exit 0
fi

# If no args better be running as a batch job, or give usage
if [ "${SLURM_JOB_ID:-}" == "" ]; then
    echo "Usage: $0 batch to submit as a batch job"
    echo
    echo "Continuing in single node mode"

#    reset_traps
#    exit 1
fi

if [ "${1:-}" == "build" ] ; then
    echo "Building SST versions"
    echo "=================================================="
    date
    push_cwd ~/Code/SST/phold

    echo Loading gcc/10
    reset_traps
    module load gcc/10
    set_traps

    build perf-old-no  perf-old --disable-mem-pools
    build perf-old-yes perf-old --disable-mem-pools \
	  --enable-event-tracking --enable-perf-tracking --enable-profile
    build perf-new profile-new  --disable-mem-pools

    echo
    echo "=============================="
    echo
    echo "Build options summary:"
    for f in profile/build*.log ; do 
	echo
	echo $f
	grep -B 1 -A 7 "(Options):" $f 
    done

    date

    reset_traps
    exit 0
fi

#  Assume the 'run' arg was given, or running in batch

echo
echo
echo "Execution performance of new and old SST profiling"
echo "=================================================="
echo
date


echo
echo "Running variants"
echo "=============================="
push_cwd ~/Code/SST/phold

for (( i=0; i<10; i++ ))
do
    run_all $i
done

echo
echo "=================================================="
echo
echo "Results:"

cd profile
for f in `ls perf-*.log`
do
    echo "$f   " `grep "Simulation time:" $f` 
done
cd -

echo
date

reset_traps
exit 0

# Results

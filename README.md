# dtcq

## Setup

### Install boost library
Boost library is used to deal with files and directories.
Installation depends on OS.
For Ubuntu, do:
```bash
sudo apt install libboost-dev libboost-all-dev
```

### General
```bash
BRANCH="v0.4" # Change to whatever branch / tag you need
git clone --recursive git@github.com:bu-cms/dtcq.git -b "${BRANCH}"
cd dtcq
mkdir build
cd build
cmake ..
make
```

### On lxplus

Preface the setup above by sourcing a recent LCG environment of your choice. E.g.:

```
source /cvmfs/sft.cern.ch/lcg/views/LCG_99/x86_64-centos7-gcc8-opt/setup.sh
```
Haven't tested this version on LXPLUS yet, especially if it has boost library in the LCG environment.

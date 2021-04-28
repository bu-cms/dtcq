# dtcq

## Setup

# General
```bash
BRANCH="dtcq" # Change to whatever branch / tag you need
git clone --recursive git@github.com:bu-cms/dtcq.git -b "${BRANCH}"
cd dtcq
git submodule update --init
mkdir build
cd build
cmake ..
make
```

# On lxplus

Preface the setup above by sourcing a recent LCG environment of your choice. E.g.:

```
source /cvmfs/sft.cern.ch/lcg/views/LCG_99/x86_64-centos7-gcc8-opt/setup.sh
```

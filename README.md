# dtcq

## Setup
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

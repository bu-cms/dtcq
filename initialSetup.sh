#!/usr/bin/env bash

git update-index --assume-unchanged initialSetup.sh
git update-index --assume-unchanged initialSetup.csh

mkdir env
cd env

source /cvmfs/sft.cern.ch/lcg/views/LCG_95apython3/x86_64-centos7-gcc8-opt/setup.sh

ENVNAME="bucoffeaenv"
python -m venv ${ENVNAME}
source ${ENVNAME}/bin/activate
export PYTHONPATH=${PWD}/${ENVNAME}/lib/python3.6/site-packages:$PYTHONPATH
git clone git@github.com:bu-cms/bucoffea.git
python -m pip install -e bucoffea
pip install matplotlib==3.1
pip install mplhep==0.1.5
cd ..

echo 'source /cvmfs/sft.cern.ch/lcg/views/LCG_95apython3/x86_64-centos7-gcc8-opt/setup.sh' >> setup.sh
echo 'ENVNAME="bucoffeaenv"' >> setup.sh
echo 'source env/${ENVNAME}/bin/activate' >> setup.sh
echo 'export PYTHONPATH=env/${ENVNAME}/lib/python3.6/site-packages:$PYTHONPATH' >> setup.sh
echo 'git update-index --assume-unchanged initialSetup.sh' >> setup.sh
echo 'git update-index --assume-unchanged initialSetup.csh' >> setup.sh

mv initialSetup.*sh env/

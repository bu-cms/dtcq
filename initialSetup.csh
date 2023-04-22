#!/usr/bin/env tsch

git update-index --assume-unchanged initialSetup.sh
git update-index --assume-unchanged initialSetup.csh

mkdir env
cd env

source /cvmfs/sft.cern.ch/lcg/views/LCG_95apython3/x86_64-centos7-gcc8-opt/setup.csh
setenv ENVNAME "bucoffeaenv"
python -m venv ${ENVNAME}
source ${ENVNAME}/bin/activate.csh
setenv PYTHONPATH ${PWD}/${ENVNAME}/lib/python3.6/site-packages:$PYTHONPATH
git clone git@github.com:bu-cms/bucoffea.git
python -m pip install -e bucoffea
pip install matplotlib==3.1
pip install mplhep==0.1.5
cd ..

echo 'source /cvmfs/sft.cern.ch/lcg/views/LCG_95apython3/x86_64-centos7-gcc8-opt/setup.csh' >> setup.csh
echo 'setenv ENVNAME "bucoffeaenv"' >> setup.csh
echo 'source env/${ENVNAME}/bin/activate.csh' >> setup.csh
echo 'setenv PYTHONPATH env/${ENVNAME}/lib/python3.6/site-packages:${PYTHONPATH}' >> setup.csh
echo 'git update-index --assume-unchanged initialSetup.sh' >> setup.csh
echo 'git update-index --assume-unchanged initialSetup.csh' >> setup.csh

mv initialSetup.*sh env/

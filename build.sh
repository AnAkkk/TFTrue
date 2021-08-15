#!/bin/bash
rm build -rf;
mkdir build;
cd build;
qmake -qt=qt5 ../TFTrue.pro -r -spec linux-g++;
make;
sync;
cp TFTrue.so /home/steph/tfTEST/tf2/tf/addons/TFTrue.so

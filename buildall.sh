#!/bin/sh

echo " exec 001_build_dist.sh ..."
./001_build_dist.sh $1
echo " exec 002_build_rpm.sh ..."
./002_build_rpm.sh

echo "done"

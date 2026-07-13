#!/bin/bash

echo "Building..."

mkdir build; cd build &&

cmake .. &&
make -j$(nproc) &&

echo "Building Complete!"

#!/bin/bash

./configure --disable-assertions --with-malloc=tcmalloc
make -j16

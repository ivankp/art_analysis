#!/bin/bash

./bin/draw_vars \
  `ls | grep '\.root' | sed -e 's/\(.*\)\.root/-i \1\:\1\.root/g'` \
  -o vars_hist.pdf -n


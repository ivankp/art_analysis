#!/bin/bash

./bin/draw_vars \
  `ls | grep '\.root' | sed -e 's/\(.*\)\.root/-i \1\:\1\.root/g'` \
  -o vars_hist.pdf -n


for x in dali monet picasso renoir vangogh
do
  ./bin/calc_vars_hist "$x"_orig_

#!/bin/bash

mkdir -p grad

for t in "train" "test"
do
  # for x in dali monet picasso renoir vangogh
  # do
  #   ./bin/calc_vars_grad grad/"$x"_"$t"_orig.root out/"$x"_"$t".txt out/$x .jpg
  #   ./bin/calc_vars_grad grad/"$x"_"$t"_fft1.root out/"$x"_"$t".txt out/"$x"_fft _1.png
  #   ./bin/calc_vars_grad grad/"$x"_"$t"_fft2.root out/"$x"_"$t".txt out/"$x"_fft _2.png
  # done

  for x in orig fft1 fft2
  do
    ./bin/draw_vars `ls grad | grep "$t"_"$x\.root" | \
      sed -e 's/^\(.*\)\(_.*_.*\)$/-i \1\:grad\/\1\2/g'` \
      -o vars_"$t"_"$x".pdf -n -c config/grad_"$x".bins
  done

done

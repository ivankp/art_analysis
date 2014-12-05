#!/bin/bash

for t in "test" #"train"
do
  for x in dali monet picasso renoir vangogh
  do
    #echo ""
    ./bin/calc_vars_color out/"$x"_"$t"_orig.root out/"$x"_"$t".txt out/$x .jpg
    ./bin/calc_vars_color out/"$x"_"$t"_fft1.root out/"$x"_"$t".txt out/"$x"_fft _1.png
    ./bin/calc_vars_color out/"$x"_"$t"_fft2.root out/"$x"_"$t".txt out/"$x"_fft _2.png
  done

  for x in orig fft1 fft2
  do
    ./bin/draw_vars `ls out | grep "$t"_"$x\.root" | \
      sed -e 's/^\(.*\)\(_.*_.*\)$/-i \1\:out\/\1\2/g'` \
      -o vars_"$t"_"$x".pdf -n -c config/"$x".bins
  done

done

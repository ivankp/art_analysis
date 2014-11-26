#!/bin/bash

in=out/$1
out=$in"_fft"

mkdir -p $out

for f in `ls $in`
do

  echo $f

  b=`echo $f | sed -r 's/\.[[:alnum:]]+$//'`

  convert $in/$f -fft +depth +adjoin $out/"$b"_%d.png
  convert $out/"$b"_0.png -auto-level -evaluate log 10000 $out/"$b"_2.png

done

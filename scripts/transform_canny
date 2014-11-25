#!/bin/bash

in=out/$1
out=$in"_canny"

mkdir -p $out

for f in `ls $in`
do

  echo $f

  b=`echo $f | sed -r 's/\.[[:alnum:]]+$//'`

  convert $in/$f -canny 0x1+10%+20% $out/"$b".png
  #convert $in/$f -canny 0x1+10%+20% -hough-lines 9x9+40 $out/"$b".mvg

done

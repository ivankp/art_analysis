#!/bin/bash

#./bin/draw_vars \
#  `ls out | grep '\.root' | grep -v 'mvg' | sed -e 's/\(.*\)\.root/-i \1\:out\/\1\.root/g'` \
#  -o vars.pdf -n

./bin/draw_vars \
  `ls out | grep '\.root' | grep 'mvg' | sed -e 's/\(.*\)_mvg\.root/-i \1\:out\/\1_mvg\.root/g'` \
  -o vars_mvg.pdf -n --logy

#!/bin/sh

cat accesspoint.html | tr -d '\n' | sed 's/\"/\\\"/g' > accesspoint-oneline.txt
cat main.html | tr -d '\n' | sed 's/\"/\\\"/g' > main-oneline.txt

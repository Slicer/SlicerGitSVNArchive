#!bin/bash

min=0
hour=0

while true
do
  echo -n .
  sleep 1m
  min=$(($min + 1))
  if ! (($min % 60)); then
    hour=$(($hour + 1))
    echo "Hours elapsed: $hour"
  fi
done

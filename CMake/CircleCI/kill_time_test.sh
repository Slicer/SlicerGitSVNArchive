#!bin/bash

min=0
hour=0

while (($min < 100))
do
  echo -n .
  sleep 1m
  min=$(($min + 1))
  if ! (($min % 60)); then
    hour=$(($hour + 1))
  fi
  # Time Displayed every 10min
  if ! (($min % 10)); then
    min_modulo=$(($min % 60))
    echo "Time elapsed: $hour:$min_modulo"
  fi
done

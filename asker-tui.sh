#!/bin/bash

fn="$1"
arr=( $2 )

echo "0) Exit without running any VM"

num=1
for name in ${arr[@]}
do
  echo "$num) $name"
  let num=$num+1
done

read -p "Enter VM number to run: " vmnum

let vmnum=$vmnum+0

rm -f "$fn"
if [ "$vmnum" -gt "$num" -o "$vmnum" -eq 0 ]; then
  vmnum=0
fi

echo $vmnum > "$fn"

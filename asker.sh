#!/bin/bash

if [ -z "$1" ]; then
    exit 0
fi

tmp=$(mktemp)
terminal --disable-server --hide-menubar --hide-borders --hide-toolbars --command="./asker-tui.sh $tmp '$1'"

ec=$(cat $tmp)
rm -f $tmp

exit $ec

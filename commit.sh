#!/bin/bash -e

if [[ "$#" -ne "1" ]] || ! grep -q '\.ltm$' <<< "$1" || ! [[ -r "$1" ]]; then
	echo "usage: $0 filename.ltm" >&2
	exit 2
fi

tar xzf "$1" -C tas
sed -i tas/config.ini -e 's/game_name=.*/game_name=runner/'
git add tas
git commit -m "$(wc -l tas/inputs | cut -d ' ' -f 1) frames" --edit

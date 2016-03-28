#!/bin/sh

mkdir -p ../translations/lang
git diff 1.6/master -- src/lang/english.txt | tail -n +5 | grep '^+' | cut -b 2- > ../translations/lang/english.txt

for f in src/lang/*.txt; do
	[ "$f" = src/lang/english.txt ] && continue
	rm -f ../translations/lang/`basename $f`
	cat ../translations/lang/english.txt | grep '^STR' | while read name text; do
		grep "^$name\b" $f >> ../translations/lang/`basename $f`
	done
done

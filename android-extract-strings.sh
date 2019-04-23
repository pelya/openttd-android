#!/bin/sh

mkdir -p ../translations/lang
git diff 1.7/master -- src/lang/english.txt | tail -n +5 | grep '^[+]' | cut -b 2- | \
grep -v "^STR_TABLET_CLOSE\b" | \
grep -v "^STR_TABLET_SHIFT\b" | \
grep -v "^STR_TABLET_CTRL\b" | \
cat > ../translations/lang/english.txt

for f in src/lang/*.txt; do
	[ "$f" = src/lang/english.txt ] && continue
	rm -f ../translations/lang/`basename $f`
	cat ../translations/lang/english.txt | grep '^STR' | while read name text; do
		grep "^$name\b" $f >> ../translations/lang/`basename $f`
	done
done

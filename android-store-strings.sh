#!/bin/sh

for f in ../translations/lang/*.txt; do
	[ "$f" = ../translations/lang/english.txt ] && continue
	out=src/lang/`basename $f`
	grep "^# Android strings" $out > /dev/null || [ -z "`tail -c 2 $out`" ] || echo >> $out
	{ grep -v "^# Android strings" $out ; echo "# Android strings" ; } > $out.new
	mv -f $out.new $out
	cat $f | grep '^STR' | while read name text; do
		[ "$name" = "STR_ABOUT_MENU_SEPARATOR" ] && continue
		{ grep -v "^$name\b" $out ; printf "%-64s%s\n" "$name" "$text" ; } > $out.new
		mv -f $out.new $out
	done
done

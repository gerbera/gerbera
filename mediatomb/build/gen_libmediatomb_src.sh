echo 'libmediatomb_a_SOURCES = \'

FILTER='use strict; while (my $line=<STDIN>) { print "$line\n"; if ($line =~ /\.[ch]c?/) { print ($line =~ s/\s+//g) . " \\\n"; } }'

find ../src | perl -e "'$FILTER'"  | sort

echo dummy.h


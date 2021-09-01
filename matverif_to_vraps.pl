#!perl

# This script converts matverif gadgets into VRAPS-v2 gadgets

use strict;
use warnings;
use feature qw(say);
use autodie qw(open close);

if (! -f $ARGV[0]) {
    say "Usage: $0 <filename>";
    exit;
}

my ($order, @randoms, @lines);
open my $FH, '<', $ARGV[0];
while (<$FH>) {
    chomp;
    if (/ORDER\s*=\s*(\d+)/) {
        $order = $1;
    } elsif (/MASKS = \[\s*(.*?)\s*\]/) {
        @randoms = split /\s*,\s*/, $1;
    } elsif (/\S/) {
        push @lines, $_;
    }
}

say "#order $order";
say "#shares ", $order+1;
say "#in a b";
say "#out c";
say "#randoms @randoms\n";

for my $i (0 .. $order) {
    for my $j (0 .. $order) {
        say "s$i$j = a$i * b$j";
    }
    say "";
}
say "";

for my $outnum (0 .. $#lines) {
    my $tmp = "t$outnum";
    my @line = split /\s+/, $lines[$outnum];
    if (@line == 2) {
        say "c$outnum = $line[0] + $line[1]";
    } else {
        say "t$outnum = $line[0] + $line[1]";
        for (2 .. @line-2) {
            say "t$outnum = t$outnum + $line[$_]";
        }
        say "c$outnum = t$outnum + $line[-1]";
    }
    say "";
}

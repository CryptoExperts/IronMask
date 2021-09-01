#!perl

# This script converts into VRAPS gadgets into SILVER gadgets

use strict;
use warnings;

my ($shares, @in, @rands, $out);

while (($_ = <>) =~ /^#/) {
    chomp;
    if (/#shares\s+(\d+)/i) { $shares = $1; }
    if (/#in\s+(.+?)\s*$/i) { @in = split / /, $1 }
    if (/#out\s+(.+?)\s*$/i) { $out = $1 }
    if (/#rand(?:om)?s\s+(.+?)\s*$/i) { @rands = split / /, $1 }
}

my %assoc;
my $instr_count = 0;

# Inputs
for my $i (0 .. $#in) {
    for my $s (0 .. $shares-1) {
        printf "in %d %d_%d #  $in[$i]\[$s]\n",
            $instr_count, $i, $s;
        $assoc{$in[$i] . $s} = $instr_count;
        $instr_count++;
    }
}

# Randoms
for my $i (0 .. $#rands) {
    printf "ref %d #  $rands[$i]\n", $instr_count;
    $assoc{$rands[$i]} = $instr_count;
    $instr_count++;
}

# Gadget body
while (defined($_ = <>)) {
    chomp;
    next if /^#/ || !/\w/;
    if (/(\w+)\s*=\s*(\w+)\s*([*+])\s*(\w+)/) {
        my ($dst, $src1, $op, $src2) = ($1, $2, $3, $4);
        printf "%s %d %d #  $_ --> %d\n",
            $op eq '+' ? 'xor' : 'and',
            $assoc{$src1}, $assoc{$src2}, $instr_count;
        $assoc{$dst} = $instr_count;
        $instr_count++;
    } elsif (/(\w+)\s*=\s*(\w+)\s*$/) {
        my ($dst, $src) = ($1, $2);
        printf "reg %d # $_ --> %d\n", $assoc{$src}, $instr_count;
        $assoc{$dst} = $instr_count;
        $instr_count++;
    }
}

# Outputs
for my $i (0 .. $shares-1) {
    my $out_name = $out . $i;
    printf "out %d 0_%d #  %s\n",
        $assoc{$out_name}, $i, $out_name;

}

#!perl

# This script converts into VRAPS gadgets into matverif gadgets

use strict;
use warnings;
use feature 'say';


my ($shares, @in, @rands, $out);

while (($_ = <>) =~ /^#/) {
    chomp;
    if (/#shares\s+(\d+)/i) { $shares = $1; }
    if (/#in\s+(.+?)\s*$/i) { @in = split / /, $1 }
    if (/#out\s+(.+?)\s*$/i) { $out = $1 }
    if (/#rand(?:om)?s\s+(.+?)\s*$/i) { @rands = split / /, $1 }
}

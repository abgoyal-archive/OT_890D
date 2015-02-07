#!/usr/bin/perl
($n) = @ARGV;
$n += 0;

while ( defined($line = <STDIN>) ) {
    if ( $line =~ /\$\$/ ) {
	$rep = $n;
    } else {
	$rep = 1;
    }
    for ( $i = 0 ; $i < $rep ; $i++ ) {
	$tmp = $line;
	$tmp =~ s/\$\$/$i/g;
	$tmp =~ s/\$\#/$n/g;
	$tmp =~ s/\$\*/\$/g;
	print $tmp;
    }
}

#!/usr/bin/perl -w
use strict;

my ($readdir, $installdir, $arch, @files) = @ARGV;

my $unifdef = "scripts/unifdef -U__KERNEL__";

foreach my $file (@files) {
	local *INFILE;
	local *OUTFILE;
	my $tmpfile = "$installdir/$file.tmp";
	open(INFILE, "<$readdir/$file")
		or die "$readdir/$file: $!\n";
	open(OUTFILE, ">$tmpfile") or die "$tmpfile: $!\n";
	while (my $line = <INFILE>) {
		$line =~ s/([\s(])__user\s/$1/g;
		$line =~ s/([\s(])__force\s/$1/g;
		$line =~ s/([\s(])__iomem\s/$1/g;
		$line =~ s/\s__attribute_const__\s/ /g;
		$line =~ s/\s__attribute_const__$//g;
		$line =~ s/^#include <linux\/compiler.h>//;
		$line =~ s/(^|\s)(inline)\b/$1__$2__/g;
		$line =~ s/(^|\s)(asm)\b(\s|[(]|$)/$1__$2__$3/g;
		$line =~ s/(^|\s|[(])(volatile)\b(\s|[(]|$)/$1__$2__$3/g;
		printf OUTFILE "%s", $line;
	}
	close OUTFILE;
	close INFILE;
	system $unifdef . " $tmpfile > $installdir/$file";
	unlink $tmpfile;
}
exit 0;

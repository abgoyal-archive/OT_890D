#!/usr/bin/perl
while (<>) {
  my $line = $_;

  $_ =~ /\W*[0-9]+\W*([a-zA-Z\_0-9]+)\W*[0-9]+/;

  if ( ($line =~ /unknown/) || ($line =~ /total/)) {

  } else {
    print "*(.text.$1)\n";
  }
}

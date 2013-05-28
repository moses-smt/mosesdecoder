#!/usr/bin/perl

use strict;

#my $cmd = "astyle --style='k&r' -s2 -v --recursive *.h *.cpp";
#print STDERR "Executing: $cmd \n";
#system($cmd);

opendir(DIR,".") or die "Can't open the current directory: $!\n";

# read file/directory names in that directory into @names 
my @names = readdir(DIR) or die "Unable to read current dir:$!\n";

foreach my $name (@names) {
   next if ($name eq ".");   # skip the current directory entry
   next if ($name eq "..");  # skip the parent  directory entry
   next if ($name eq "boost");  # skip the parent  directory entry
   next if ($name eq "contrib");  # skip the parent  directory entry
   next if ($name eq "jam-files");  # skip the parent  directory entry
   next if ($name eq ".git");  # skip the parent  directory entry

   if (-d $name){            # is this a directory?
      my $cmd = "astyle --style='k&r' -s2 -v --recursive $name/*.h $name/*.cpp";
      print STDERR "Executing: $cmd \n";
      system($cmd);

      next;                  # can skip to the next name in the for loop 
   }
}

closedir(DIR);

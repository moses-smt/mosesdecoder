#!/usr/bin/perl -w
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# utility script for deploying decode (may be within tune) jobs over a cluster
# with NFS-mounted drives. copy all the model files to local disk.

use strict;

my $CAT_MODELS = 0;

die("ERROR: syntax is cache-model.perl moses.ini cache-dir") 
  unless scalar @ARGV >= 2;
my $CONFIG = $ARGV[0];
my $CACHE_DIR = $ARGV[1];
if (scalar(@ARGV) == 3) {
    $CAT_MODELS = $ARGV[2];
}

# create dir (if nor already there)
`mkdir -p $CACHE_DIR`;

# name for new config file
my $cached_config = $CONFIG;
$cached_config =~ s/\//_/g;
$cached_config = "$CACHE_DIR/$cached_config";

# lock / already
while(-e "$cached_config.lock") {
  sleep(10);
}
my $just_update_timestamps = (-e $cached_config);
`touch $cached_config.lock` unless $just_update_timestamps;

# find files to cache (and produce new config)
open(OLD,$CONFIG) || die("ERROR: could not open config '$CONFIG'");
open(NEW,">$cached_config") unless $just_update_timestamps;
while(<OLD>) {
  if (/(PhraseDictionary.+ path=)(\S+)(.*)$/ ||
      /(LexicalReordering.+ path=)(\S+)(.*)$/ ||
      /(Generation.+ path=)(\S+)(.*)$/ ||
      /(OpSequenceModel.+ path=)(\S+)(.*)$/ ||
      /(KENLM.+ path=)(\S+)(.*)$/) {
    my ($pre,$path,$post) = ($1,$2,$3);
    my $new_path;
    if (/^PhraseDictionaryCompact/) {
      $new_path = &cache_file($path,".minphr", $CAT_MODELS);
    }
    elsif (/^PhraseDictionaryBinary/) {
      foreach my $suffix (".binphr.idx",".binphr.srctree.wa",".binphr.srcvoc",".binphr.tgtdata.wa",".binphr.tgtvoc") {
        $new_path = &cache_file($path,$suffix, $CAT_MODELS);
      }
    }
    elsif (/^LexicalReordering/ && -e "$path.minlexr") {
      $new_path = &cache_file($path,".minlexr", $CAT_MODELS);
    }
    elsif (/^LexicalReordering/ && -e "$path.binlexr.idx") {
      foreach my $suffix (".binlexr.idx",".binlexr.srctree",".binlexr.tgtdata",".binlexr.voc0",".binlexr.voc1") {
        $new_path = &cache_file($path,$suffix, $CAT_MODELS);
      }
    }
    # some other files may need some more special handling
    # but this works for me right now. feel free to add
    else {  
      $new_path = &cache_file($path,"", $CAT_MODELS);
    }
    print NEW "$pre$new_path$post\n" unless $just_update_timestamps;
  }
  else {
    print NEW $_ unless $just_update_timestamps;
  }
}
close(NEW) unless $just_update_timestamps;
close(OLD);

`rm $cached_config.lock` unless $just_update_timestamps;
print "$cached_config\n";

sub cache_file {
  my ($path,$suffix, $catModels) = @_;

  # add gzipped extension if that's what it is 
  if (! -e "$path$suffix" && -e "$path$suffix.gz") {
    $suffix .= ".gz";
  }

  # file does not exist... nothing to do
  if (! -e "$path$suffix") {
    print STDERR "WARINING: $path$suffix does not exist - cannot be cached by cache-model.perl\n";
    return $path;
  }

  # follow symbolic link
  my $uniq_path = `readlink -f $path$suffix`;
  chop($uniq_path);

  # create cached file name
  my $cached_path = $uniq_path;   
  $cached_path = substr($cached_path,0,length($cached_path)-length($suffix));
  $cached_path =~ s/\//_/g;
  $cached_path = "$CACHE_DIR/$cached_path";

  # sleep if another process is copying right now...
  while(-e "$cached_path$suffix.lock") {
    sleep(10);
  }
  # done if already there
  if (-e "$cached_path$suffix") {
    `touch $cached_path$suffix`; # update time stamp
  }
  else {
    # okay, go for it
    `touch $cached_path$suffix.lock`;
    `cp -r $path$suffix $cached_path$suffix`;
    `rm $cached_path$suffix.lock`;
  }

  if ($catModels) {
      my $cmd = "cat $cached_path* > /dev/null";
      print STDERR "Executing: $cmd\n";
      `$cmd`;
  }
  return $cached_path;
}


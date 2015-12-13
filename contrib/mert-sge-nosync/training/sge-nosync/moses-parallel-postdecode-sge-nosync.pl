#!/usr/bin/perl


my $logflag="";
my $logfile="";
my $alifile=undef;
my $nbestflag=0;
my $processid=0;
my $idxliststr="";
my $workingdir="";
my $inputfile="";
my $tmpdir="";
my $splitpfx="";
my $jobscript="";
my $qsubout="";
my $qsuberr="";
my $nbestfile=undef;
my $nbestlist=undef;
my $outnbest="";
my $lsamp_filename="";
my @idxlist=();


###############################
# Script starts here

init();



#concatenating translations and removing temporary files
concatenate_1best();
concatenate_logs() if $logflag;
concatenate_ali() if defined $alifile;
concatenate_nbest() if $nbestflag;
safesystem("cat nbest$$ >> /dev/stdout") if $nbestlist[0] eq '-';


print STDERR "Not support searchgraphflag for sync mert\n" if $searchgraphflag;
# concatenate_searchgraph() if $searchgraphflag;  
# safesystem("cat searchgraph$$ >> /dev/stdout") if $searchgraphlist eq '-';

print STDERR "Not support wordgraphflag for sync mert\n" if $searchgraphflag;
# concatenate_wordgraph() if $wordgraphflag;  
# safesystem("cat wordgraph$$ >> /dev/stdout") if $wordgraphlist[0] eq '-';

remove_temporary_files();
####
#### ### ending scripts in run_decoder() ##############
#### sanity_check_order_of_lambdas($featlist, $filename);
#### ## how to do return???
#### return ($filename, $lsamp_filename);
######################################################




sub init(){
  use strict;
  use Getopt::Long qw(:config pass_through no_ignore_case permute);

  GetOptions('alignment-output-file=s'=>\$alifile,
             'process-id=s'=>\$processid,
             'idxliststr=s'=>\$idxliststr,
             'logfile=s'=>\$logfile,
	     'nbestfile=s'=>\$nbestfile,
             'outnbest=s'=>\$outnbest,
             'lsamp-filename=s'=>\$lsamp_filename,
             'input-file=s'=>\$inputfile
           ) or exit(1);

  if ($logfile){ $logflag=1; }

  if (defined $nbestfile) { $nbestflag=1; }

  $idxliststr =~ s/^\s+|\s+$//g;
  @idxlist = split(/\s+/,$idxliststr);
  
  my $pwdcmd = getPwdCmd();

  $workingdir = `$pwdcmd`; chomp $workingdir;
  $tmpdir="$workingdir/tmp$processid";
  $splitpfx="split$processid";

  $jobscript="$workingdir/job$processid";
  $qsubout="$workingdir/out.job$processid";
  $qsuberr="$workingdir/err.job$processid";

  # print STDERR "$idxliststr\n";

}


sub concatenate_nbest(){
  my $oldcode="";
  my $newcode=-1;
  my %inplength = ();
  my $offset = 0;

# get the list of feature and set a fictitious string with zero scores
  open (IN, "${nbestfile}.${splitpfx}$idxlist[0]");
  my $str = <IN>;
  chomp($str);
  close(IN);
  my ($code,$trans,$featurescores,$globalscore)=split(/\|\|\|/,$str);

  my $emptytrans = "  ";
  my $emptyglobalscore = " 0.0";
  my $emptyfeaturescores = $featurescores;
  $emptyfeaturescores =~ s/[-0-9\.]+/0/g;

  if ($outnbest eq '-'){ $outnbest="nbest$processid"; }

  # my $outnbest=$nbestlist[0];
  # if ($nbestlist[0] eq '-'){ $outnbest="nbest$$"; }

  open (OUT, "> $outnbest");
  foreach my $idx (@idxlist){

#computing the length of each input file
    # print STDERR "this idx: $idx\n";

    my @in=();
    open (IN, "${inputfile}.${splitpfx}${idx}.trans");
    @in=<IN>;
    close(IN);
    $inplength{$idx} = scalar(@in);

    open (IN, "${nbestfile}.${splitpfx}${idx}");
    while (<IN>){
      my ($code,@extra)=split(/\|\|\|/,$_);
      $code += $offset;
      if ($code ne $oldcode){
# if there is a jump between two consecutive codes
# it means that an input sentence is not translated
# fill this hole with a "fictitious" list of translation
# comprising just one "emtpy translation" with zero scores
        while ($code - $oldcode > 1){
           $oldcode++;
           print OUT join("\|\|\|",($oldcode,$emptytrans,$emptyfeaturescores,$emptyglobalscore)),"\n";
        }
      }
      $oldcode=$code;
      print OUT join("\|\|\|",($oldcode,@extra));
    }
    close(IN);
    $offset += $inplength{$idx};

    while ($offset - $oldcode > 1){
      $oldcode++;
      print OUT join("\|\|\|",($oldcode,$emptytrans,$emptyfeaturescores,$emptyglobalscore)),"\n";
    }
  }
  close(OUT);
}


sub concatenate_1best(){
  foreach my $idx (@idxlist){
    # print STDERR "reading 1best file ${inputfile}.${splitpfx}$idx.trans\n";
    my @in=();
    open (IN, "${inputfile}.${splitpfx}${idx}.trans");
    @in=<IN>;
    # print STDERR "in array is : @in";
    print STDOUT "@in";
    close(IN);
  }
}

sub concatenate_logs(){
  open (OUT, "> ${logfile}");
  foreach my $idx (@idxlist){
    my @in=();
    open (IN, "$qsubout$idx");
    @in=<IN>;
    print OUT "@in";
    close(IN);
  }
  close(OUT);
}

sub concatenate_ali(){
  open (OUT, "> ${alifile}");
  foreach my $idx (@idxlist){
    my @in=();
    open (IN, "$alifile.$splitpfx$idx");
    @in=<IN>;
    print OUT "@in";
    close(IN);
  }
  close(OUT);
}




# look for the correct pwdcmd (pwd by default, pawd if it exists)
# I assume that pwd always exists
sub getPwdCmd(){
        my $pwdcmd="pwd";
        my $a;
        chomp($a=`which pawd | head -1 | awk '{print $1}'`);
        if ($a && -e $a){       $pwdcmd=$a;     }
        return $pwdcmd;
}


sub remove_temporary_files(){
  # removing temporary files
  foreach my $idx (@idxlist){
    unlink("${inputfile}.${splitpfx}${idx}.trans");
    unlink("${inputfile}.${splitpfx}${idx}");
    if (defined $alifile){ unlink("${alifile}.${splitpfx}${idx}"); }
    if ($nbestflag){ unlink("${nbestfile}.${splitpfx}${idx}"); }
    if ($searchgraphflag){ unlink("${searchgraphfile}.${splitpfx}${idx}"); }
    if ($wordgraphflag){ unlink("${wordgraphfile}.${splitpfx}${idx}"); }

    # print STDERR "Deleting ${jobscript}${idx}.bash\n";
    unlink("${jobscript}${idx}.bash");
    unlink("${jobscript}${idx}.log");
    unlink("$qsubname.W.log");
    unlink("$qsubout$idx");
    unlink("$qsuberr$idx");
    rmdir("$tmpdir");
  }
  # unlink("${jobscript}.sync_workaround_script.sh");
  if ($nbestflag && $nbestlist[0] eq '-'){ unlink("${nbestfile}$$"); };
  if ($searchgraphflag  && $searchgraphlist eq '-'){ unlink("${searchgraphfile}$$"); };
  if ($wordgraphflag  && $wordgraphlist eq '-'){ unlink("${wordgraphfile}$$"); };
}






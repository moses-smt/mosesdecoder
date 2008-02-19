#! /usr/bin/perl

# $Id$
#Converts direct and inverted alignments into a more compact 
#bi-alignment format. It optionally reads the counting file 
#produced by giza containing the frequency of each traning sentence.

#Copyright Marcello Federico, November 2004

($cnt,$dir,$inv)=();

while ($w=shift @ARGV){
  $dir=shift(@ARGV),next  if $w eq "-d";
  $inv=shift(@ARGV),next  if $w eq "-i";
  $cnt=shift(@ARGV),next  if $w eq "-c";
} 

my $lc = 0;

if (!$dir || !inv){
 print  "usage: giza2bal.pl [-c <count-file>] -d <dir-align-file> -i <inv-align-file>\n"; 
 print  "input files can be also commands, e.g. -d \"gunzip -c file.gz\"\n";
 exit(0);
}

$|=1;

open(DIR,"<$dir") || open(DIR,"$dir|") || die "cannot open $dir\n";
open(INV,"<$inv") || open(INV,"$inv|") || die "cannot open $dir\n";

if ($cnt){
open(CNT,"<$cnt") || open(CNT,"$cnt|") || die "cannot open $dir\n";
}


sub ReadBiAlign{
    local($fd0,$fd1,$fd2,*s1,*s2,*a,*b,*c)=@_;
    local($dummy,$n);

    chop($c=<$fd0>); ## count
    $dummy=<$fd0>; ## header
    $dummy=<$fd0>; ## header
    $c=1 if !$c;

    $dummy=<$fd1>; ## header
    chop($s1=<$fd1>);
    chop($t1=<$fd1>);

    $dummy=<$fd2>; ## header
    chop($s2=<$fd2>);
    chop($t2=<$fd2>);

    @a=@b=();
    $lc++;

    #get target statistics
    $n=1;
    $t1=~s/NULL \(\{((\s+\d+)*)\s+\}\)//;
    while ($t1=~s/(\S+)\s+\(\{((\s+\d+)*)\s+\}\)//){
        grep($a[$_]=$n,split(/\s+/,$2));
        $n++;
    }

    $m=1;
    $t2=~s/NULL \(\{((\s+\d+)*)\s+\}\)//;
    while ($t2=~s/(\S+)\s+\(\{((\s+\d+)*)\s+\}\)//){
        grep($b[$_]=$m,split(/\s+/,$2));
        $m++;
    }

    $M=split(/\s+/,$s1);
    $N=split(/\s+/,$s2);

    if ($m != ($M+1) || $n != ($N+1)) {
      print STDERR "Sentence mismatch error! Line #$lc\n";
      $s1 = "ALIGN_ERR";
      $s2 = "ALIGN_ERR";
      @a=(); @b=();
      for ($j=1;$j<2;$j++){ $a[$j]=1; }
      for ($i=1;$i<2;$i++){ $b[$i]=1; }
      return 1;
    }

    for ($j=1;$j<$m;$j++){
        $a[$j]=0 if !$a[$j];
    }

    for ($i=1;$i<$n;$i++){
        $b[$i]=0 if !$b[$i];
    }


    return 1;
}

$skip=0;
$ccc=0;
while(!eof(DIR)){

    if (ReadBiAlign(CNT,DIR,INV,*src,*tgt,*a,*b,*c))
    {
	$ccc++;
        print "$c\n";
        print $#a," $src \# @a[1..$#a]\n";
        print $#b," $tgt \# @b[1..$#b]\n";
    }
    else{
    	print "\n";
        print STDERR "." if !(++$skip % 1000);
    }
};
print STDERR "skip=<$skip> counts=<$ccc>\n";

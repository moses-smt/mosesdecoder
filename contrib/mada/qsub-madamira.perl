#!/usr/bin/env perl

use warnings;
use strict;
use File::Slurp;
use File::Basename;
use Cwd 'abs_path';

my $splitDir = $ARGV[0];
$splitDir = abs_path($splitDir);

my @files = read_dir $splitDir;

my $qsubDir=dirname($splitDir) ."/qsub";
print STDERR "qsubDir=$qsubDir\n";
`mkdir -p $qsubDir`;

my $out2Dir=dirname($splitDir) ."/out2";
print STDERR "out2Dir=$out2Dir\n";
`mkdir -p $out2Dir`;

for my $file ( @files ) {
    print STDERR "$file ";

    my $qsubFile = "$qsubDir/$file.sh";
    open(RUN_FILE, ">$qsubFile");
    
    print RUN_FILE "#!/usr/bin/env bash\n" 
	."#PBS -d/scratch/hh65/workspace/experiment/ar-en \n"
        ."#PBS -l mem=5gb \n\n"
	."export PATH=\"/scratch/statmt/bin:/share/apps/NYUAD/perl/gcc_4.9.1/5.20.1/bin:/share/apps/NYUAD/jdk/1.8.0_31/bin:/share/apps/NYUAD/zlib/gcc_4.9.1/1.2.8/bin:/share/apps/NYUAD/cmake/gcc_4.9.1/3.1.0-rc3/bin:/share/apps/NYUAD/boost/gcc_4.9.1/openmpi_1.8.3/1.57.0/bin:/share/apps/NYUAD/openmpi/gcc_4.9.1/1.8.3/bin:/share/apps/NYUAD/python/gcc_4.9.1/2.7.9/bin:/share/apps/NYUAD/gcc/binutils/2.21/el6/bin:/share/apps/NYUAD/gcc/gcc/4.9.1/el6/bin:/usr/lib64/qt-3.3/bin:/usr/local/bin:/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/sbin:/opt/bio/ncbi/bin:/opt/bio/mpiblast/bin:/opt/bio/EMBOSS/bin:/opt/bio/clustalw/bin:/opt/bio/tcoffee/bin:/opt/bio/hmmer/bin:/opt/bio/phylip/exe:/opt/bio/mrbayes:/opt/bio/fasta:/opt/bio/glimmer/bin:/opt/bio/glimmer/scripts:/opt/bio/gromacs/bin:/opt/bio/gmap/bin:/opt/bio/tigr/bin:/opt/bio/autodocksuite/bin:/opt/bio/wgs/bin:/opt/ganglia/bin:/opt/ganglia/sbin:/opt/bin:/usr/java/latest/bin:/opt/pdsh/bin:/opt/rocks/bin:/opt/rocks/sbin:/opt/torque/bin:/opt/torque/sbin:/home/hh65/bin:/home/hh65/bin\" \n"

	."module load  NYUAD/2.0 \n"
	."module load gcc python/2.7.9 openmpi/1.8.3 boost cmake zlib jdk perl expat \n"

	."cd /scratch/statmt/MADAMIRA-release-20140709-1.0 \n";
    print RUN_FILE "java -Xmx2500m -Xms2500m -XX:NewRatio=3 -jar /scratch/statmt/MADAMIRA-release-20140709-1.0/MADAMIRA.jar "
	 ."-rawinput $splitDir/$file -rawoutdir $out2Dir -rawconfig /scratch/statmt/MADAMIRA-release-20140709-1.0/samples/sampleConfigFile.xml \n";

    close(RUN_FILE);

    my $cmd = "qsub $qsubFile";
    `$cmd`;

}


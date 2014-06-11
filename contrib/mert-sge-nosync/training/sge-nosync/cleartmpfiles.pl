#!/usr/bin/perl


    my @filename_id = "";
    my $this_id = "";

    # remove exitjob and forceexitjob
    chomp(my @rddfile_list = `ls exitjob* forceexitjob*`);
    foreach my $rddfile (@rddfile_list) {
      unlink("$rddfile");
    }

    chomp(@filename_id = `ls *.id | grep -v 'clear'`);
    open (OUT, "> all.id.all");
    print OUT "==Combine log at ".`date`;
    print OUT `tail -n +1 *.id`;
    print OUT "==LOG combined ".`date`;
    close(OUT);
    foreach $this_id (@filename_id) {
      # print OUT `cat $this_id`;
      unlink("$this_id");
    }
    
    chomp (@filename_id = `ls *.id.pid | grep -v 'clear'`);
    open (OUT, "> all.id.pid.all");
    print OUT "==Combine log at ".`date`;
    print OUT `tail -n +1 *.id.pid`;
    print OUT "==Log combined ".`date`;
    close(OUT);
    foreach $this_id (@filename_id) {
      # print OUT `cat $this_id`;
      unlink("$this_id");
    }



    chomp(@filename_id = `ls *.out | grep -v 'clear'`);
    open (OUT, "> all.out.all");
    print OUT "==Combine log at ".`date`;
    print OUT `tail -n +1 *.out`;
    print OUT "==Log combined ".`date`;
    close(OUT); 
    foreach $this_id (@filename_id) {
      # print OUT `cat $this_id`;
      unlink("$this_id");
    }
    
    chomp(@filename_id = `ls *.err | grep -v 'clear'`);
    open (OUT, "> all.err.all");
    print OUT "==Combine log at ".`date`;
    print OUT `tail -n +1 *.err`;    
    print OUT "==Log combined ".`date`;
    close(OUT);
    foreach $this_id (@filename_id) {
      # print OUT `cat $this_id`;
      unlink("$this_id");
    }
    
    # waitall.sh which cannot be deleted inside moses-parallel-sge-nosync.pl
    chomp(@filename_id = `ls *waitall.sh`);
    foreach $this_id (@filename_id) {
      unlink("$this_id");
    }


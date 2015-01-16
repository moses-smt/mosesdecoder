#!/usr/bin/perl -w

# Sample Tokenizer
### Version 1.1
# written by Pidong Wang, based on the code written by Josh Schroeder and Philipp Koehn
# Version 1.1 updates:
#       (1) add multithreading option "-threads NUM_THREADS" (default is 1);
#       (2) add a timing option "-time" to calculate the average speed of this tokenizer;
#       (3) add an option "-lines NUM_SENTENCES_PER_THREAD" to set the number of lines for each thread (default is 2000), and this option controls the memory amount needed: the larger this number is, the larger memory is required (the higher tokenization speed);
### Version 1.0
# $Id: tokenizer.perl 915 2009-08-10 08:15:49Z philipp $
# written by Josh Schroeder, based on code by Philipp Koehn

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

use FindBin qw($RealBin);
use strict;
use Time::HiRes;
use Thread;

my $mydir = "$RealBin/../share/nonbreaking_prefixes";

my %NONBREAKING_PREFIX = ();
my $language = "en";
my $QUIET = 0;
my $HELP = 0;
my $AGGRESSIVE = 0;
my $SKIP_XML = 0;
my $TIMING = 0;
my $NUM_THREADS = 1;
my $NUM_SENTENCES_PER_THREAD = 2000;

while (@ARGV) 
{
	$_ = shift;
	/^-b$/ && ($| = 1, next);
	/^-l$/ && ($language = shift, next);
	/^-q$/ && ($QUIET = 1, next);
	/^-h$/ && ($HELP = 1, next);
	/^-x$/ && ($SKIP_XML = 1, next);
	/^-a$/ && ($AGGRESSIVE = 1, next);
	/^-time$/ && ($TIMING = 1, next);
	/^-threads$/ && ($NUM_THREADS = int(shift), next);
	/^-lines$/ && ($NUM_SENTENCES_PER_THREAD = int(shift), next);
}

# for time calculation
my $start_time;
if ($TIMING)
{
    $start_time = [ Time::HiRes::gettimeofday( ) ];
}

# print help message
if ($HELP) 
{
	print "Usage ./tokenizer.perl (-l [en|de|...]) (-threads 4) < textfile > tokenizedfile\n";
        print "Options:\n";
        print "  -q     ... quiet.\n";
        print "  -a     ... aggressive hyphen splitting.\n";
        print "  -b     ... disable Perl buffering.\n";
        print "  -time  ... enable processing time calculation.\n";
	exit;
}

if (!$QUIET) 
{
	print STDERR "Tokenizer Version 1.1\n";
	print STDERR "Language: $language\n";
	print STDERR "Number of threads: $NUM_THREADS\n";
}

# load the language-specific non-breaking prefix info from files in the directory nonbreaking_prefixes
load_prefixes($language,\%NONBREAKING_PREFIX);

if (scalar(%NONBREAKING_PREFIX) eq 0)
{
	print STDERR "Warning: No known abbreviations for language '$language'\n";
}

my @batch_sentences = ();
my @thread_list = ();
my $count_sentences = 0;

if ($NUM_THREADS > 1)
{# multi-threading tokenization
    while(<STDIN>) 
    {
        $count_sentences = $count_sentences + 1;
        push(@batch_sentences, $_);
        if (scalar(@batch_sentences)>=($NUM_SENTENCES_PER_THREAD*$NUM_THREADS))
        {
            # assign each thread work
            for (my $i=0; $i<$NUM_THREADS; $i++)
            {
                my $start_index = $i*$NUM_SENTENCES_PER_THREAD;
                my $end_index = $start_index+$NUM_SENTENCES_PER_THREAD-1;
                my @subbatch_sentences = @batch_sentences[$start_index..$end_index];
                my $new_thread = new Thread \&tokenize_batch, @subbatch_sentences;
                push(@thread_list, $new_thread);
            }
            foreach (@thread_list)
            {
                my $tokenized_list = $_->join;
                foreach (@$tokenized_list)
                {
                    print $_;
                }
            }
            # reset for the new run
            @thread_list = ();
            @batch_sentences = ();
        }
    }
    # the last batch
    if (scalar(@batch_sentences)>0)
    {
        # assign each thread work
        for (my $i=0; $i<$NUM_THREADS; $i++)
        {
            my $start_index = $i*$NUM_SENTENCES_PER_THREAD;
            if ($start_index >= scalar(@batch_sentences))
            {
                last;
            }
            my $end_index = $start_index+$NUM_SENTENCES_PER_THREAD-1;
            if ($end_index >= scalar(@batch_sentences))
            {
                $end_index = scalar(@batch_sentences)-1;
            }
            my @subbatch_sentences = @batch_sentences[$start_index..$end_index];
            my $new_thread = new Thread \&tokenize_batch, @subbatch_sentences;
            push(@thread_list, $new_thread);
        }
        foreach (@thread_list)
        {
            my $tokenized_list = $_->join;
            foreach (@$tokenized_list)
            {
                print $_;
            }
        }
    }
}
else
{# single thread only
    while(<STDIN>) 
    {
        if (($SKIP_XML && /^<.+>$/) || /^\s*$/) 
        {
            #don't try to tokenize XML/HTML tag lines
            print $_;
        }
        else 
        {
            print &tokenize($_);
        }
    }
}

if ($TIMING)
{
    my $duration = Time::HiRes::tv_interval( $start_time );
    print STDERR ("TOTAL EXECUTION TIME: ".$duration."\n");
    print STDERR ("TOKENIZATION SPEED: ".($duration/$count_sentences*1000)." milliseconds/line\n");
}

#####################################################################################
# subroutines afterward

# tokenize a batch of texts saved in an array
# input: an array containing a batch of texts
# return: another array cotaining a batch of tokenized texts for the input array
sub tokenize_batch
{
    my(@text_list) = @_;
    my(@tokenized_list) = ();
    foreach (@text_list)
    {
        if (($SKIP_XML && /^<.+>$/) || /^\s*$/) 
        {
            #don't try to tokenize XML/HTML tag lines
            push(@tokenized_list, $_);
        }
        else
        {
            push(@tokenized_list, &tokenize($_));
        }
    }
    return \@tokenized_list;
}

# the actual tokenize function which tokenizes one input string
# input: one string
# return: the tokenized string for the input string
sub tokenize 
{
    my($text) = @_;

    #clean some stuff so you don't get &amp; -> &amp;amp;
    #news-commentary stuff
    
    $text =~ s/\&#45;/ /g;
    $text =~ s/\&45;/ /g;
    $text =~ s/\&#160;/ /g;
    $text =~ s/\&gt;/\>/g;
    $text =~ s/\&lt;/\</g;
    $text =~ s/ampquot;/\"/g;
    $text =~ s/ampquot/\"/g;
    $text =~ s/\&quot;/\"/g;
    $text =~ s/\&amp;/\&/g;
    $text =~ s/\&nbsp;/ /g;
    $text =~ s/\&#91;/\[/g;   # syntax non-terminal
    $text =~ s/\&#93;/\]/g;   # syntax non-terminal
    $text =~ s/\&bar;/\|/g;   # factor separator (legacy)
    $text =~ s/\&#124;/\|/g;  # factor separator
    $text =~ s/(\.){4,}/ /g; #remove junk like ........
    $text =~ s/--/ -- /g;

    chomp($text);
    $text = " $text ";
    
    # remove ASCII junk
    $text =~ s/\s+/ /g;
    $text =~ s/[\000-\037]//g;

    # seperate out all "other" special characters
    $text =~ s/([^\p{IsAlnum}\s\.\'\`\,\-])/ $1 /g;

    # aggressive hyphen splitting
    if ($AGGRESSIVE) 
    {
        $text =~ s/([\p{IsAlnum}])\-([\p{IsAlnum}])/$1 \@-\@ $2/g;
    }

    #multi-dots stay together
    $text =~ s/\.([\.]+)/ DOTMULTI$1/g;
    while($text =~ /DOTMULTI\./) 
    {
        $text =~ s/DOTMULTI\.([^\.])/DOTDOTMULTI $1/g;
        $text =~ s/DOTMULTI\./DOTDOTMULTI/g;
    }

    # seperate out "," except if within numbers (5,300)
    $text =~ s/([^\p{IsN}])[,]([^\p{IsN}])/$1 , $2/g;
    # separate , pre and post number
    $text =~ s/([\p{IsN}])[,]([^\p{IsN}])/$1 , $2/g;
    $text =~ s/([^\p{IsN}])[,]([\p{IsN}])/$1 , $2/g;
	      
    # turn `into '
    #$text =~ s/\`/\'/g;
	
    #turn '' into "
    #$text =~ s/\'\'/ \" /g;

    if ($language eq "en") 
    {
        #split contractions right
        # $text =~ s/ [']([\p{IsAlpha}])/ '$1/g; #MARIA: is pretokenized for parsing vb'll -> vb 'll
        $text =~ s/([Dd])'ye/$1o you/g;
        $text =~ s/([Dd])'you/$1o you/g;
        $text =~ s/'Tis/It is/g;
        $text =~ s/'tis/it is/g;
        $text =~ s/'Twas/It was/g;
        $text =~ s/'twas/it is/g;
        $text =~ s/([^\p{IsAlpha}])[']([^\p{IsAlpha}])/$1 ' $2/g;
        $text =~ s/([^\p{IsAlpha}\p{IsN}])[']([\p{IsAlpha}])/$1 ' $2/g;
        $text =~ s/([\p{IsAlpha}])['][ ]([sSmMdDtT]\s)/$1 '$2/g; # Commissioner' s
        $text =~ s/([\p{IsAlpha}])['][ ](ll|ve) /$1 '$2 /g; # I' ve I' ll
        $text =~ s/ ['] ([sSmMdDtT]\s)/ '$1/g; # Maria 's -> Maria ' s -> Maria 's
        $text =~ s/ ['] (ll|ve) / '$1 /g; # I 'll I 've
        $text =~ s/([\p{IsAlpha}])[']([^\p{IsAlpha}])/$1 ' $2/g;
        $text =~ s/([\p{IsAlpha}])[']([\p{IsAlpha}])/$1 '$2/g;
        #$text =~ s/ ['] ([\p{IsAlpha}])/ '$1/g; # I 'll 1999 's
        $text =~ s/([\p{IsAlpha}])n [']t/$1 n't/g; #don't -> do n't (don't first splits into don 't)
        $text =~ s/([\p{IsAlpha}])n ['] t/$1 n't/g;
        $text =~ s/([\p{IsAlpha}])n  [']t/$1 n't/g;
        $text =~ s/([\p{IsAlpha}])N [']T/$1 N'T/g;
        #special case for "1990's"
        $text =~ s/([\p{IsN}])[']s/$1 's/g;
        $text =~ s/([\p{IsN}]) [']s/$1 's/g;
        $text =~ s/([\p{IsN}]) [']  s/$1 's/g;
        
        

        #other english contractions -> from PTB tokenizer.sed
        $text =~ s/([Cc])annot/$1an not/g;
        $text =~ s/([Gg])imme/$1im me/g;
        $text =~ s/([Gg])onna/$1on na/g;
        $text =~ s/([Gg])otta/$1ot ta/g;
        $text =~ s/([Ll])emme/$1em me/g;
        $text =~ s/([Ww])anna/$1an na/g;
        $text =~ s/([Dd]) 'ye/$1' ye/g;
        
    } 
    elsif (($language eq "fr") or ($language eq "it")) 
    {
        #split contractions left	
        $text =~ s/([^\p{IsAlpha}])[']([^\p{IsAlpha}])/$1 ' $2/g;
        $text =~ s/([^\p{IsAlpha}])[']([\p{IsAlpha}])/$1 ' $2/g;
        $text =~ s/([\p{IsAlpha}])[']([^\p{IsAlpha}])/$1 ' $2/g;
        $text =~ s/([\p{IsAlpha}])[']([\p{IsAlpha}])/$1' $2/g;
    } 
    else 
    {
        $text =~ s/\'/ \' /g;
    }
	
    #word token method
    my @words = split(/\s/,$text);
    $text = "";
    for (my $i=0;$i<(scalar(@words));$i++) 
    {
        my $word = $words[$i];
        if ( $word =~ /^(\S+)\.$/) 
        {
            my $pre = $1;
            if (($pre =~ /\./ && $pre =~ /\p{IsAlpha}/) || ($NONBREAKING_PREFIX{$pre} && $NONBREAKING_PREFIX{$pre}==1) || ($i<scalar(@words)-1 && ($words[$i+1] =~ /^[\p{IsLower}]/))) 
            {
                #no change
			} 
            elsif (($NONBREAKING_PREFIX{$pre} && $NONBREAKING_PREFIX{$pre}==2) && ($i<scalar(@words)-1 && ($words[$i+1] =~ /^[0-9]+/))) 
            {
                #no change
            } 
            else 
            {
                $word = $pre." .";
            }
        }
        $text .= $word." ";
    }		

    # clean up extraneous spaces
    $text =~ s/ +/ /g;
    $text =~ s/^ //g;
    $text =~ s/ $//g;

    #restore multi-dots
    while($text =~ /DOTDOTMULTI/) 
    {
        $text =~ s/DOTDOTMULTI/DOTMULTI./g;
    }
    $text =~ s/DOTMULTI/./g;

    #escape special chars
    $text =~ s/\&/\&amp;/g;   # escape escape
    $text =~ s/\|/\&#124;/g;  # factor separator
    $text =~ s/\</\&lt;/g;    # xml
    $text =~ s/\>/\&gt;/g;    # xml
    $text =~ s/\'/\&apos;/g;  # xml
    $text =~ s/\"/\&quot;/g;  # xml
    $text =~ s/\[/\&#91;/g;   # syntax non-terminal
    $text =~ s/\]/\&#93;/g;   # syntax non-terminal

    #ensure final line break
    $text .= "\n" unless $text =~ /\n$/;

    return $text;
}

sub load_prefixes 
{
    my ($language, $PREFIX_REF) = @_;
	
    my $prefixfile = "$mydir/nonbreaking_prefix.$language";
	
    #default back to English if we don't have a language-specific prefix file
    if (!(-e $prefixfile)) 
    {
        $prefixfile = "$mydir/nonbreaking_prefix.en";
        print STDERR "WARNING: No known abbreviations for language '$language', attempting fall-back to English version...\n";
        die ("ERROR: No abbreviations files found in $mydir\n") unless (-e $prefixfile);
    }
	
    if (-e "$prefixfile") 
    {
        open(PREFIX, "<:utf8", "$prefixfile");
        while (<PREFIX>) 
        {
            my $item = $_;
            chomp($item);
            if (($item) && (substr($item,0,1) ne "#")) 
            {
                if ($item =~ /(.*)[\s]+(\#NUMERIC_ONLY\#)/) 
                {
                    $PREFIX_REF->{$1} = 2;
                } 
                else 
                {
                    $PREFIX_REF->{$item} = 1;
                }
            }
        }
        close(PREFIX);
    }
}


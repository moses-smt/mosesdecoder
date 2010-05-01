# file: Subprocess.pm

# Herve Saint-Amand
# Universitaet des Saarlandes
# Wed May 14 09:55:46 2008

# NOTE that to use this with Philipp Koehn's tokenizer.perl I had to modify
# that script to autoflush its streams, by adding a '$|++' to it

#------------------------------------------------------------------------------
# includes

package Subprocess;

use warnings;
use strict;

use Encode;
use IPC::Open2;

#------------------------------------------------------------------------------
# constructor

sub new {
    my ($class, @cmd) = @_;
    bless {
        cmd => \@cmd,
        num_done => 0,
        child_in  => undef,
        child_out => undef,
    }, $class;
}

#------------------------------------------------------------------------------

sub start {
    my ($self) = @_;
    open2 ($self->{child_out}, $self->{child_in}, @{$self->{cmd}});
}

sub do_line {
    my ($self, $line) = @_;
    my ($in, $out) = ($self->{child_in}, $self->{child_out});

    $line =~ s/\s+/ /g;
    print $in encode ('UTF-8', $line), "\n";
    $in->flush ();

    my $ret = decode ('UTF-8', scalar <$out>);
    chomp $ret;

    $self->{num_done}++;
    return $ret;
}

sub num_done { shift->{num_done} }

#------------------------------------------------------------------------------

1;


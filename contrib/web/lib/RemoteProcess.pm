# file: RemoteProcess.pm

# Herve Saint-Amand
# Universitaet des Saarlandes
# Thu May 15 08:30:19 2008

#------------------------------------------------------------------------------
# includes

package RemoteProcess;
our @ISA = qw/Subprocess/;

use warnings;
use strict;

use IO::Socket::INET;

use Subprocess;

#------------------------------------------------------------------------------
# constructor

sub new {
    my ($class, $host, $port) = @_;

    my $self = new Subprocess;
    $self->{host} = $host;
    $self->{port} = $port;
    $self->{sock} = undef;

    bless $self, $class;
}

#------------------------------------------------------------------------------
# should have the same interface as Subprocess.pm

sub start {
    my ($self) = @_;

    $self->{sock} = new IO::Socket::INET (%{{
        PeerAddr => $self->{host},
        PeerPort => $self->{port},
    }}) || die "Can't connect to $self->{host}:$self->{port}";

    $self->{child_in} = $self->{child_out} = $self->{sock};
}

#------------------------------------------------------------------------------

1;


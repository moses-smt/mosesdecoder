package LMClient;

use IO::Socket;

sub new {
  my ($class, $cstr) = @_;
  my $self = {};
  $cstr =~ s/^!//;
  my ($host, $port) = split /\:/, $cstr;
  die "Please specify connection string as host:port" unless ($host && $port);

  $self->{'SOCK'} = new IO::Socket::INET(
    PeerAddr => $host,
    PeerPort => $port,
    Proto => 'tcp') or die "Couldn't create connection to $host:$port -- is memcached running?\n";

  bless $self, $class;
  return $self;
}

sub word_prob {
  my ($self, $word, $context) = @_;
  my @cwords = reverse split /\s+/, $context;
  my $qstr = "prob $word @cwords";
  my $s = $self->{'SOCK'};
  print $s "$qstr\r\n";
  my $r = <$s>;
  my $x= unpack "f", $r;
  return $x;
}

sub close {
  my ($self) = @_;
  close $self->{'SOCK'};
}

1;

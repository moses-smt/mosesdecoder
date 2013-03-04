use Win32::Registry;
use Getopt::Long;
use Archive::Extract;

my ($regname);
my ($file);

$result = GetOptions ("regname=s" => \$regname, "file=s"   => \$file);
my ($rkey);
my ($var);
my ($type);
my ($p) = 'SOFTWARE\Moses Core Team\MosesDecoder\Models';
$HKEY_CURRENT_USER->Open($p, $rkey);
$rkey->QueryValueEx($regname, $type, $var) if $rkey;
system("$file -y -o\"$var\"") if $var && $file =~ /.exe$/;
Archive::Extract->new(archive => $file)->extract(to => $var) if $var && $file =~ /.zip$/;

#find the installation path
my ($install);
my ($rkey2);
$HKEY_CURRENT_USER->Open('SOFTWARE\Moses Core Team\MosesDecoder', $rkey2);
$rkey2->QueryValueEx("Path", $type, $install) if $rkey2;
my ($runfile);
$runfile = $var."run.bat" if $install;
print $runfile;
if ($install && -e $runfile) {
	open FILE, $runfile;
	@contents = <FILE>;
	close FILE;
	open FILE, ">$runfile";
	print FILE "\@echo off\nset PATH=%PATH%;\"$install\"\n";
	print FILE @contents;
	close FILE;
}

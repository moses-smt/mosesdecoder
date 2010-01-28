#!/usr/bin/perl -Tw
use warnings;
use strict;
$|++;

# file: translate.cgi

# Herve Saint-Amand
# saintamh [o] yahoo, com
# Universitaet des Saarlandes
# Mon May 12 14:10:54 2008

# This CGI script takes a web page URL as a parameter, fetches that page,
# translates it using the Moses decoder, and displays the translated version
# to the user, similarily to how Google or BabelFish translate web pages.

# I don't think I've ever written anything with such a high comment/code ratio,
# so hopefully it should be understandable. Just read top to bottom.


# TODO:
# 
#  - if the document contains <a name='anchor'></a> it will be lost
#  - don't insert spaces everywhere around soft tags
#  - charset autodetection would be nice, but it's not trivial

#------------------------------------------------------------------------------
# includes

use CGI;
use CGI::Carp qw/fatalsToBrowser/;

# we use the 2nd perl thread API. I think this means you need perl 5.6 or
# higher, compiled with thread support
use threads;
use threads::shared;

use Encode;
use HTML::Entities;
use HTML::Parser;
use LWP::UserAgent;
use URI;
use URI::Escape;

use lib 'lib';
use RemoteProcess;
use Subprocess;

#------------------------------------------------------------------------------
# constants, config

# In order to run this script, you must first start Moses as a sort of daemon
# process that accepts connections on some INET port, reads the sentences sent
# to it one line at a time and returns translations. The daemon.pl script that
# comes with this script does just that -- starts an instance of Moses and
# 'plugs' it to the net so it can be used from other machines or just other
# processes on the same machine.
# 
# This list here indicates where to find these instances of Moses. May be 
# localhost, or may be separate machines.
# 
# On the current UniSaar setup we use SSH tunneling to connect to other hosts,
# so from this script's POV they're all localhost. These ports are actually
# forwarded to other machines. There wouldn't be much point in running 16
# instances of Moses on the same machine.

my @MOSES_ADDRESSES = map "localhost:90$_",
    qw/01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16/;


# The tokenizer tries to adapt its rules depending on the language it's dealing
# with, so we indicate that here.

my $INPUT_LANG  = 'fr';
my $OUTPUT_LANG = 'en';


# In order to tokenize and detokenize strings in a way that stays consistent
# with how it is done in the rest of the Moses system, we use the scripts that
# come with Moses as external processes. These are the commands we must run to
# start them.

my @TOKENIZER_CMD   = ('./bin/tokenizer.perl',   '-l', $INPUT_LANG);
my @DETOKENIZER_CMD = ('./bin/detokenizer.perl', '-l', $OUTPUT_LANG);


# We call 'soft tags' HTML tags whose presence is tolerated inside
# sentences. All other tags are assumed to be sentence-breakers and will be
# used to chop up documents into independent sentences. These few, however, are
# allowed within sentences.

my %SOFT_TAGS = map {$_ => 1} qw/a b i u em font blink tt acronym/;


# We call 'verbatim tags' HTML tags whose entire data is to be left untouched
# and reprinted as-is. These also happen to be tags whose content is typically
# not printed by the browser.

my %VERBATIM_TAGS = map {$_ => 1} qw/script style/;


# Some HTML tags have attributes that contain URLs. Since we'll be displaying
# the page on another server than its usual source server, relative paths will
# be broken, so we need to make all URLs absolute. These are the attributes
# that will be so modified.

my %URL_ATTRS = %{{
    a      => 'href',
    img    => 'src',
    form   => 'action',
    link   => 'href',
    script => 'src',
}};


# Some HTML tags have attributes that can contain free text that is displayed
# to the user. Data in attributes is not usually translated, but these should
# be.
# 
# Note that for implementation reasons these will always be treated as hard,
# sentence-splitting tags. This could be changed but would require a
# substantial re-write of this script.

my %TEXT_ATTR = %{{ input => [qw/value/], img => [qw/alt title/], }};


# Sentence splitting within a paragraph or block of text is done after
# tokenizing. Tokens matched by this regex will be considered to end a
# sentence, and hence be used in splitting the text into sentences.

my $RE_EOS_TOKEN = qr/^(?:\.+|[\?!:;])$/;

# This regex also matches sentence-ending tokens, but tokens matched by this
# one will not be included in the sentence itself. Tokens matched by the
# previous regex will be sent to Moses as part of the end of the sentence.
# Tokens matches by this one will never be sent to Moses. Which is why the pipe
# symbol, which Moses doesn't seem to like, must be in here.

my $RE_SPLIT_TOKEN = qr!^[\|\-]+$!;

#------------------------------------------------------------------------------
# global vars

# In cleaner code there wouldn't be global variables, but it simplified things
# to put these here. Eventually I wouldn't mind removing this section.


# This array is very central to the way this script works. The document will be
# chopped up into a list of 'segments'. Each segment is either some HTML code
# and whitespace which we don't translate or manipulate in any way, or a bit of
# text to be translated. It's as if we highlighted in the HTML source the bits
# of text that needed translation, and make each stripe of highlighter, and
# each length of text between them, a segment.
# 
# Segments that are untouched HTML are simply strings. If the whole document
# contained no translatable text, this array would only contain strings.
# 
# Segments that contain text to be translated are represented as arrayrefs. The
# first element of that arrayref is the text to be translated, with any soft
# tags within it replaced by placeholders of the type MOSESOPENTAG4. The
# remaining elements contain the necessary info to reinsert these tags. The
# placeholders are numbered, and the i-th placeholder corresponds to the
# (i+1)-th element in the arrayref (element 0 being the text). That element is
# itself an array ref, whose first element is the tag name and second element
# is a hashref of attributes.
# 
# So this document:
# 
#   <p>This is <a href="somewhere">a link</a> but it's not <b>bold</b></p>
# 
# would be represented by this @segments array:
# 
#   0: "<p>"
#   1: [ 0: "This is MOSESOPENTAG0 a link MOSESCLOSETAG0 but it's not" .
#           " MOSESOPENTAG1 bold MOSESCLOSETAG1"
#        1: [ "a", { href => "somewhere" } ]
#        2: [ "b", {} ] ]
#   2: "</p>"
# 
# Finally, there's one hack to be mentioned: text in %TEXT_ATTR attributes
# (defined above) also goes into a segment of its own. Since this text does
# not contain tags, and to signal that the code for the popup containing
# source text should not be inserted around this text, we replace the tag
# information by the "__NOPOPUP__" string. So this document:
# 
#   <img src="blah" alt="This describes the image">
# 
# would correspond to this @segments array:
# 
#   0: "<img src=\"blah\" alt=\""
#   1: [ "This describes the image", "__NOPOPUP__" ]
#   2: "\">"
# 
# This is a horrible hack. Yes.

my @segments;


# Finally, since this script is run in 'tainted' mode (-T switch) for basic
# security reasons, and we'll be launching subprocesses, so we need to make
# sure the PATH is clean otherwise Perl will refuse to do the system() calls.

$ENV{PATH} = '';

#------------------------------------------------------------------------------
# Fetch the source page

# get value of URL param, make sure it's absolute
my $url = CGI->new->param ('url');
die "No URL?" unless $url;
$url = "http://$url" unless ($url =~ m!^[a-z]+://!);

# configure Web client
my $lwp = new LWP::UserAgent (%{{
    agent   => $ENV{HTTP_USER_AGENT} || 'Mozilla/5.0',
    timeout => 5,
}});

# fetch the web page we want to translate
my $res = $lwp->get ($url);
die "Couldn't fetch page: " . $res->status_line unless $res->is_success;
my $html = $res->decoded_content;

# Find the page's base url. It may be different than the URL given to us as
# parameter if for instance that URL redirects to a different one, or the
# document contains a <base> tag.
my $base_url = $res->base;

# Decode entities, except some basics because it confuses our parsing. We need
# this because Moses won't understand the entities. It sometimes introduces
# minor display bugs, though. TODO: decode only alphanumerical entities?
$html =~ s/&((?:lt|gt);?)/&amp;$1/g;
$html = decode_entities ($html);

# Start printing HTML page
print "Content-Type: text/html; charset=UTF-8\n\n";

#------------------------------------------------------------------------------
# Parser stack and state management

# We're going to use a callback parser to parse the HTML file. As we walk the
# HTML tree we maintain a buffer containing the current block if text to be
# translated. These state variables contain that. The buffer is repeatedly
# emptied and its contents pushed onto @segments.
# 
# We also remove 'soft' tags from the text as we append it to the buffer,
# replace them with placeholders, and save info about the tags we set aside in
# @buf_tag_index. @buf_tag_stack keeps track of 'currently open' tags, so that
# we can match closing tags to their opening tags.

my $buf_text_has_content = 0;
my $buf_text = '';
my @buf_tag_index;
my @buf_tag_stack;

my $in_verbatim = 0;


# This is called when we find soft tags within text to be translated. Arguments
# are the tag name, a hash of tag attributes, and a boolean telling us whether
# it's an opening or closing tag.
# 
# We perform lookups in the above state variables, save the tag info in them if
# necessary, and return a string which is the placeholder to replace that tag.

sub make_placeholder {
    my ($tag, $attr, $closing) = @_;
    my $placeholder = '';

    if ($closing) {

        # try to match closing tags with their opening sibling
        foreach my $i (reverse 0 .. $#buf_tag_stack) {
            if ($buf_tag_stack[$i][0] eq $tag) {
                $placeholder = 'MOSESCLOSETAG' . $buf_tag_stack[$i][1];
                splice (@buf_tag_stack, $i, 1);
                last;
            }
        }

        # lone closing tags are added to the index but not the stack
        if (!$placeholder) {
            push (@buf_tag_index, [ $tag, $attr ]);
            $placeholder = 'MOSESCLOSETAG' . $#buf_tag_index;
        }

    } else {
        # opening tags are added to the index and the stack
        push (@buf_tag_index, [ $tag, $attr ]);
        push (@buf_tag_stack, [ $tag, $#buf_tag_index ]);
        $placeholder = 'MOSESOPENTAG' . $#buf_tag_index;
    }

    return $placeholder;
}


# When we hit a hard tag, we call this to save any current text segment we have
# to the @segments array.

sub flush_buf_text {
    if ($buf_text_has_content || @buf_tag_index) {
        push (@segments, [ $buf_text, @buf_tag_index ] );
    } else {
        push (@segments, $buf_text);
    }

    $buf_text = '';
    @buf_tag_index = ();
    @buf_tag_stack = ();
    $buf_text_has_content = 0;
}

#------------------------------------------------------------------------------
# HTML parser

# Parser callback for when we hit an opening or closing tag
sub start_and_end_h {
    my ($tag, $attr, $closing) = @_;

    # keep track of whether we're in a verbatim segment
    $in_verbatim = $closing ? 0 : $tag
        if $VERBATIM_TAGS{$tag};

    # make links absolute
    my $url_attr = $URL_ATTRS{$tag};
    &make_link_absolute ($tag, $attr, $url_attr)
        if ($url_attr && $attr->{$url_attr});

    # textual attributes require some trickery - FIXME this duplicates some of
    # &print_tag
    if ($TEXT_ATTR{$tag}) {
        &flush_buf_text ();
        my $found = 0;

        # there's an example of how this works in the comments that precede the
        # declaration of @segments, above
        foreach my $text_attr (@{$TEXT_ATTR{$tag}}) {
            if ($attr->{$text_attr}) {
                push (@segments, ($found ? '"' : "<$tag") . " $text_attr=\"");
                push (@segments, [ $attr->{$text_attr}, '__NOPOPUP__' ]);
                delete $attr->{$text_attr};
                $found = 1;
            }
        }

        if ($found) {
            my $self_close = delete $attr->{'/'} ? 1 : 0;
            push (@segments, "\"" . join ('', map {
                (my $v = $attr->{$_}) =~ s/\"/&\#34;/g;
                " $_=\"$v\"";
            } keys %{$attr}) . ($self_close ? ' /' : '') . '>');
        } else {
            push (@segments, &print_tag ($tag, $attr, $closing));
        }

    # if the tag is soft we buffer it, if it's hard we flush the buffer out
    } elsif ($SOFT_TAGS{$tag}) {
        my $placeholder = &make_placeholder ($tag, $attr, $closing);
        $buf_text .= ' ' . $placeholder . ' ';
    } else {
        &flush_buf_text ();
        push (@segments, &print_tag ($tag, $attr, $closing));
    }

    # add a <base> tag at the beginning of the <head> (do we need this?)
    push (@segments, "<base src='$base_url'>\n")
        if ($tag eq 'head' && !$closing);
}


# parser callback for text segments
sub text_h {
    my ($text) = @_;

    if ($in_verbatim) {
        # when in verbatim mode (in <script> or <style> tags), everything just
        # gets reprinted as-is

        # .. except this
        $text =~ s/\@import\s+\"([^\n\"]+)\"/
            '@import "' . URI->new_abs($1, $base_url)->as_string . '"';
        /ge;

        push (@segments, $text);

    } else {
        # otherwise add the text to the sentence buffer
        $buf_text .= $text;
        $buf_text_has_content ||= ($text =~ /\p{IsAlnum}/);
    }
}

sub rest_h {
    my ($text) = @_;
    &flush_buf_text ();
    push (@segments, $text);
}


my $parser = HTML::Parser->new (%{{
    start_h       => [\&start_and_end_h, 'tagname, attr' ],
    text_h        => [\&text_h,          'text'          ],
    declaration_h => [\&rest_h,          'text'          ],
    comment_h     => [\&rest_h,          'text'          ],

    end_h => [sub {
        &start_and_end_h (shift, {}, 1);
    }, 'tagname' ],
}});

# parse it into @segments
$parser->parse ($html);
undef $parser;

#------------------------------------------------------------------------------
# Run translation threads

# We have now parsed the who document to the @segments array. Now we start
# the actual translation process.
# 
# We start one thread for each Moses host defined in the configuration above.
# All threads will then race to translate text segments, working down the
# @segments array. They also print segments as soon as a sequence of segments
# is done.


# These are the variables that are shared between threads and used for
# synchronisation.

my @input  :shared = map { ref $_ ? $_->[0] : undef } @segments;
my @output :shared = map { ref $_ ? undef : $_ } @segments;
my $next_job_i :shared = 0;
my $num_printed :shared = 0;


# This sub will be run in parallel by the threads
my $thread_body = sub {
    my ($moses_i) = @_;

    # each thread uses it's own tokenizer and detokenizer subprocess
    # (FIXME -- isn't this hugely inefficient?)
    my $tokenizer   = new Subprocess (@TOKENIZER_CMD);
    my $detokenizer = new Subprocess (@DETOKENIZER_CMD);
    $tokenizer->start;
    $detokenizer->start;

    # each thread also connects to its own Moses server
    my ($host, $port) = split /:/, $MOSES_ADDRESSES[$moses_i];
    my $moses = new RemoteProcess ($host, $port) ||
        die "Can't connect to '$host:$port'";
    $moses->start;

    for (;;) {

        # Snatch the next unassigned job from the queue
        my $job_i;
        { lock $next_job_i; $job_i = $next_job_i++; }
        last if ($job_i > $#input);

        # If it's a text job, translate it, otherwise just don't do anything
        $output[$job_i] = &translate_text_with_placeholders
            ($input[$job_i], $moses, $tokenizer, $detokenizer)
            if (!defined $output[$job_i]);

        # Print out any sequential block of done jobs
        lock $num_printed;
        while ($num_printed < @input && defined $output[$num_printed]) {
            my $print;

            if (ref $segments[$num_printed]) {

                # replace placeholders by the original tags
                my @buf_tag_index = @{$segments[$num_printed]};
                shift @buf_tag_index;
                $print = &replace_placeholders_by_tags
                    ($output[$num_printed], @buf_tag_index);

                # wrap in code to popup the original text onmouseover
                if ($buf_tag_index[0] ne '__NOPOPUP__') {
                    $print = &add_original_text_popup
                        ($input[$num_printed], $print);
                } else {
                    $print =~ s/\"/&\#34;/g;
                }

            } else {
                # HTML segments are just printed as-is
                $print = $segments[$num_printed];
            }

            print encode ('UTF-8', $print);
            $num_printed++;
        }
    }
};

if (@MOSES_ADDRESSES == 1) {

    # If there's only one instance of Moses, there's no point in forking a
    # single thread and waiting for it to complete, so we just run the thread
    # code directly in the main thread
    $thread_body->(0);

} else {

    # Start all threads and wait for them all to finish
    my @threads = map {
        threads->create ($thread_body, $_);
    } (0 .. $#MOSES_ADDRESSES);
    $_->join foreach @threads;

}

#------------------------------------------------------------------------------
# Translation subs


# This sub is called bt the translation thread for each text segment. The
# arguments are the input text and pointers to the various external processes
# needed for processing.
# 
# At this stage the input text contains placeholders that look like
# "MOSESOPENTAG2". We don't need to know which tag they stand for, but we do
# need to set them aside, translate the remaining plain text, and reinsert them
# at the correct place in the translation.

sub translate_text_with_placeholders {
    my ($input_text, $moses, $tokenizer, $detokenizer) = @_;
    my $traced_text = '';

    # Start by tokenizing the text, with placeholders still in it. The
    # placeholders are designed to be interpreted as individual tokens by the
    # tokenizer.
    my @tokens = split /\s+/, $tokenizer->do_line ($input_text);

    # remove placeholders, and for each remaining token, make a list of the
    # tags that cover it
    @tokens = ('START', @tokens, 'END');
    my @tags_over_token = &_extract_placeholders (\@tokens);
    @tokens = @tokens[1 .. $#tokens-1];

    # translate sentence by sentence
    my $token_base_i = 0;
    while (@tokens > 0) {

        # take a string of tokens up to the next sentence-ending token
        my (@s_tokens, $split_token);
        while (@tokens > 0) {
            if ($tokens[0] =~ $RE_EOS_TOKEN) {
                push (@s_tokens, shift @tokens);
                last;
            } elsif ($tokens[0] =~ $RE_SPLIT_TOKEN) {
                $split_token = shift @tokens;
                last;
            } else {
                push (@s_tokens, shift @tokens);
            }
        }

        # Join together tokens into a plain text string. This is now ready to
        # be shipped to Moses: all tags and placeholders have been removed,
        # and it's a single sentence. We also lowercase as needed, and make
        # a note of whether we did.
        my $s_input_text = join (' ', @s_tokens);
        my $was_ucfirst =
            ($s_input_text =~ s/^(\p{IsUpper})(?=\p{IsLower})/lc $1;/e);
        my $was_allcaps =
            ($s_input_text =~ s/^([\p{IsUpper}\P{IsAlpha}]+)$/lc $1;/e);

        # Translate the plain text sentence
        # my $s_traced_text = &_translate_text_pig_latin ($s_input_text);
        my $s_traced_text = &_translate_text_moses ($s_input_text, $moses);

        # Early post-translation formatting fixes
        $s_traced_text .= " $split_token" if $split_token;
        $s_traced_text = ucfirst $s_traced_text if $was_ucfirst;
        $s_traced_text = uc      $s_traced_text if $was_allcaps;

        # Update trace numbers to fit in the Grand Scheme of Things
        $s_traced_text =~ s{\s*\|(\d+)-(\d+)\|}{
            ' |' . ($1+$token_base_i) . '-' . ($2+$token_base_i) . '| ';
        }ge;
        $token_base_i += @s_tokens + ($split_token ? 1 : 0);

        $traced_text .= $s_traced_text . ' ';
    }

    # Apply to every segment in the traced output the union of all tags
    # that covered tokens in the corresponding source segment
    my $output_text = &_reinsert_placeholders
        ($traced_text, @tags_over_token);

    # Try to remove spaces inserted by the tokenizer
    $output_text = $detokenizer->do_line ($output_text);

    return $output_text;
}


# This sub takes an array of tokens, some of which are placeholders for
# formatting tags. Some of these tag placeholders are for opening tags, some
# are for closing tags. What we do here is we remove all these placeholders
# from the list and create an index of which of the remaining tokens are
# covered by which tags (by which we mean, inside their scope).
# 
# So for instance if the given array looks like this:
# 
#     [ "MOSESOPENTAG0", "MOSESOPENTAG1", "Hello", "MOSESCLOSETAG1",
#       "MOSESOPENTAG2", "world", "MOSESCLOSETAG2", "MOSESCLOSETAG0" ]
# 
# after executing this sub the array will look like this:
# 
#     [ "Hello", "world" ]
# 
# and the @tags_over_token index will have been created, containing this:
# 
#     [ [0,1], [0,2] ]
# 
# indicating that the first token ("Hello") is covered by tags 0 and 1, and
# that the 2nd token ("world") is covered by tags 0 and 2.

sub _extract_placeholders {
    my ($tokens) = @_;
    my @tags_over_token = ([]);

    while (@tags_over_token <= @$tokens) {
        my $i = $#tags_over_token;
        my @t = @{$tags_over_token[$i]};

        if ($tokens->[$i] =~ /^MOSESOPENTAG(\d+)$/) {
            $tags_over_token[$i] = [@t, $1];
            splice (@{$tokens}, $i, 1);
        } elsif ($tokens->[$i] =~ /^MOSESCLOSETAG(\d+)$/) {
            if (grep $_ == $1, @t) {
                $tags_over_token[$i] = [grep $_ != $1, @t];
            } else {
                push (@{$tags_over_token[$_]}, $1) foreach (0 .. $i-1);
            }
            splice (@{$tokens}, $i, 1);
        } else {
            push (@tags_over_token, [@t]);
        }
    }

    return @tags_over_token;
}


# This sub does pretty much the opposite of the preceding sub. It gets as
# argument the traced text output by Moses and the @tags_over_token array
# computed by the preceding sub. The traced text looks something like this:
# 
#   Hallo |0-0| Welt |1-1|
# 
# For each such segment which is between two traces, we will want to apply
# to it the union of all tags that were over the corresponding source text.
# 
# This sub does that, and returns the string, minus traces, plus reinserted
# placeholders.

sub _reinsert_placeholders {
    my ($traced_text, @tags_over_token) = @_;

    my %cur_open_tags = map {$_ => 1} @{$tags_over_token[0]};
    my $output_text = '';

    while ($traced_text =~ s/^(.+?)\s*\|(\d+)-+(\d+)\|\s*//) {
        my ($segment, $from, $to) = ($1, $2+1, $3+1);

        # list all tags that cover the source segment
        my %segment_tags = map {$_ => 1} map {
            @{$tags_over_token[$_]};
        } ($from .. $to);

        $output_text .= " MOSESCLOSETAG$_ "
            foreach (grep !$segment_tags{$_}, keys %cur_open_tags);
        $output_text .= " MOSESOPENTAG$_ "
            foreach (grep !$cur_open_tags{$_}, keys %segment_tags);
        %cur_open_tags = %segment_tags;

        $output_text .= " $segment ";
    }

    my %final_tags = map {$_ => 1} @{$tags_over_token[-1]};
    $output_text .= " MOSESCLOSETAG$_ "
        foreach (grep !$final_tags{$_}, keys %cur_open_tags);
    $output_text .= " MOSESOPENTAG$_ "
        foreach (grep !$cur_open_tags{$_}, keys %final_tags);

    $output_text .= $traced_text;
    return $output_text;
}


# Finally this one replaces the placeholders by the actual tags.

sub replace_placeholders_by_tags {
    my ($buf_text, @buf_tag_index) = @_;

    # replace the placeholders by the original tags
    $buf_text =~ s{MOSES(OPEN|CLOSE)TAG(\d+)}{
        &print_tag (@{$buf_tag_index[$2]}, $1 eq 'CLOSE');
    }ge;

    return $buf_text;
}

#------------------------------------------------------------------------------
# Interfaces to actual plain-text translators. These take a plain string and
# return a traced (Moses-style) translation


# This sub is used when you want to debug everything in this script except the
# actual translation. Translates to Pig Latin.

sub _translate_text_pig_latin {
    my ($text) = @_;

    $text =~ s/\b([bcdfhj-np-tv-z]+)([a-z]+)/
        ($1 eq ucfirst $1 ? ucfirst $2 : $2) .
        ($2 eq lc $2 ? lc $1 : $1) .
        'ay';
    /gei;

    # insert fake traces
    my $i = -1;
    $text .= ' ';
    $text =~ s/\s+/$i++; " |$i-$i| "/ge;

    return $text;
}


# This one, given a handle to a Moses subprocess, will use that to translate
# the text. Not much to see here actually.

sub _translate_text_moses {
    my ($text, $moses) = @_;

    my $traced_text = $moses->do_line ($text);
    unless ($traced_text) {
        my @tokens = split /\s+/, $text;

        # insert a fake trace if for some reason moses didn't return one
        # (which most likely indicates something is quite wrong)
        $traced_text = $text . " |0-$#tokens|";
    }

    return $traced_text;
}

#------------------------------------------------------------------------------
# basic HTML manipulation subs

sub make_link_absolute {
    my ($tag_name, $attr_hash, $attr_name) = @_;

    # make it absolute
    $attr_hash->{$attr_name} = URI->new_abs
        ($attr_hash->{$attr_name}, $base_url)->as_string;

    # make it point back to us if it's a link
    if ($tag_name eq 'a') {
        $attr_hash->{$attr_name} = 'index.cgi?url=' .
            uri_escape ($attr_hash->{$attr_name});
        $attr_hash->{target} = '_top';
    }
}

sub print_tag {
    my ($tag_name, $attr_hash, $closing) = @_;
    my $self_close = $attr_hash->{'/'} ? 1 : 0;

    return '<' . ($closing ? '/' : '') . $tag_name .
        ($closing ? '' : join ('', map {
            my $v = $attr_hash->{$_};
            $v =~ s/\"/&\#34;/g;
            " $_=\"$v\"";
        } keys %{$attr_hash})) .
        ($self_close ? ' /' : '') . '>';
}

sub add_original_text_popup {
    my ($input_text, $output_html) = @_;

    $input_text =~ s/\"/&\#34;/g;
    $input_text =~ s/MOSES(?:OPEN|CLOSE)TAG\d+//g;
    $input_text =~ s/^\s+//;
    $input_text =~ s/\s+$//;
    $input_text =~ s/\s+/ /g;

    # Using this technique for displaying the source text pop-up means we don't
    # have to fiddle with JavaScript, but it also means you need the LongTitles
    # extension installed if using Firefox.. *I* happen to have it, so..
    return "<span title=\"$input_text\">$output_html</span>";
}

#------------------------------------------------------------------------------
# conclusion

# stop the top frame counter
my $num_sentences = grep ref $_, @segments;
print "<script> top.numSentences = $num_sentences </script>\n";

#------------------------------------------------------------------------------

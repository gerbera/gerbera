#!/usr/bin/perl

use XML::DOM;
use XML::XQL;
use XML::XQL::DOM;


binmode STDOUT, ":utf8";

sub my_tag_compression
{
    my ($tag, $elem) = @_;
    
    # Print empty br, hr and img tags like this: <br />
    return 2 if $tag =~ /^(br|hr|img)$/;
    
    # Print other empty tags like this: <empty></empty>
    return 1;
}

XML::DOM::setTagCompression (\&my_tag_compression);

my $parser = new XML::DOM::Parser;
#my $doc = $parser->parsefile ("readme.html");
my $doc = $parser->parse(\*STDIN);

my @res = $doc->xql('//div[h1/@class="title"]');

$res[0]->getParentNode()->removeChild($res[0]);

@res = $doc->xql("/html/body/div/*");

foreach (@res)
{
    print $_->toString();
}


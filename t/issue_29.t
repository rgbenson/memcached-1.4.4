#!/usr/bin/perl

use strict;
use Test::More;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

if (testing_experimental_eviction()) {
    plan skip_all => "experimental eviction has no chunks";
} else {
    plan tests => 4;
}

print $sock "set issue29 0 0 0\r\n\r\n";
is (scalar <$sock>, "STORED\r\n", "stored issue29");

my $first_stats  = mem_stats($sock, "slabs");
my $first_used = $first_stats->{"1:used_chunks"};

is(1, $first_used, "Used one");

print $sock "set issue29_b 0 0 0\r\n\r\n";
is (scalar <$sock>, "STORED\r\n", "stored issue29_b");

my $second_stats  = mem_stats($sock, "slabs");
my $second_used = $second_stats->{"1:used_chunks"};

is(2, $second_used, "Used two")

#!/usr/bin/perl
# Test reallocation

use strict;
use Test::More;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $supports_experimental_eviction = supports_experimental_eviction();

if ($supports_experimental_eviction) {
    plan tests => 307;
} else {
    plan tests => 1;
    eval {
        my $server = new_memcached("-E");
    };
    ok($@, "Died with illegal -E arg when experimental eviction is not built in.");
    exit 0;
}

my $server = new_memcached("-m 3 -E");
my $sock = $server->sock;
my $long_value = "B"x66560;
my $short_value = "W"x32768;
my $key = 0;

# These aren't set to expire.
for ($key = 0; $key < 50; $key++) {
    print $sock "set key$key 0 0 66560\r\n$long_value\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored key$key - no ttl");
}

# These ones would expire in 600 seconds.
for ($key = 50; $key < 100; $key++) {
    print $sock "set key$key 0 600 66560\r\n$long_value\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored key$key - with ttl");
}

my $stats_settings = mem_stats($sock, "settings");
my $experimental_eviction_setting = $stats_settings->{"experimental_eviction"};
is($experimental_eviction_setting, "on", "check experimental eviction enabled");
my $stats  = mem_stats($sock, "items");
my $evicted = $stats->{"items:1:evicted"};
isnt($evicted, "0", "check evicted");
my $evicted_nonzero = $stats->{"items:1:evicted_nonzero"};
isnt($evicted_nonzero, "0", "check evicted_nonzero");

my $first_item_count = $stats->{"items:1:number"};
isnt($first_item_count, "0", "check item count");

# Insert more long values
for ($key = 100; $key < 150; $key++) {
    print $sock "set key$key 0 600 66560\r\n$long_value\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored key$key - more values with ttl");
}

my $stats = mem_stats($sock, "items");
my $second_item_count = $stats->{"items:1:number"};
is($first_item_count, $second_item_count, "second item count is same as first");

# Insert short values
for ($key = 150; $key < 250; $key++) {
#    print "set key$key 0 600 32768\n";
    print $sock "set key$key 0 600 32768\r\n$short_value\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored key$key - short values");
}

# Make sure we stored more of the items
my $stats = mem_stats($sock, "items");
my $third_item_count = $stats->{"items:1:number"};
cmp_ok($third_item_count, '>', $second_item_count,
       'with smaller values, more objects are stored');

# Insert more long values, assert that we store the same number as before
# Insert more long values
for ($key = 100; $key < 150; $key++) {
    print $sock "set key$key 0 600 66560\r\n$long_value\r\n";
    is(scalar <$sock>,
       "STORED\r\n", "stored key$key - more values with ttl");
}
my $stats = mem_stats($sock, "items");
my $fourth_item_count = $stats->{"items:1:number"};
is($fourth_item_count, $second_item_count,
   "with longer values, same objects are stored as with long values before")

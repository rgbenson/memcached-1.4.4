#!/usr/bin/perl
# Test reallocation

use strict;
use Test::More;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;


my $server = new_memcached("-m 3");
my $sock = $server->sock;

if (testing_experimental_eviction()) {
   plan tests => 8;
   my $stats = mem_stats($sock, "settings");
   is($stats->{"experimental_eviction"}, "on");
   is($stats->{"experimental_eviction_alloc_tries"}, "500");

   print $sock "settings experimental_eviction_alloc_tries 499\r\n";
   is(scalar <$sock>, "OK SET experimental_eviction_alloc_tries\r\n", "set alloc tries valid");
   my $stats = mem_stats($sock, "settings");
   is($stats->{"experimental_eviction_alloc_tries"}, "499");

   print $sock "settings experimental_eviction_alloc_tries 0\r\n";
   is(scalar <$sock>, "CLIENT_ERROR value out of range\r\n", "set alloc tries invalid");
   my $stats = mem_stats($sock, "settings");
   is($stats->{"experimental_eviction_alloc_tries"}, "499");

   print $sock "settings experimental_eviction_alloc_tries boosh\r\n";
   is(scalar <$sock>, "CLIENT_ERROR value must be numeric\r\n", "set alloc tries nonnumeric fails");

   print $sock "settings experimental_eviction_alloc_tries 31337\r\n";
   is(scalar <$sock>, "CLIENT_ERROR value out of range\r\n", "set alloc tries too high fails");
} else {
  plan tests => 2;
  my $stats = mem_stats($sock, "settings");
  is($stats->{"experimental_eviction"}, "off");
  print $sock "settings experimental_eviction_alloc_tries 600\r\n";
  is(scalar <$sock>, "CLIENT_ERROR experimental_eviction is not enabled\r\n",
            "don't allow setting of experimental_eviction_alloc_tries");
}

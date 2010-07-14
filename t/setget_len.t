#!/usr/bin/perl

use strict;
use Test::More tests => 14;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

my @result;

# add moo
#
print $sock "add moo 0 0 35\r\nhelp_me_I_am_tired_of_this_nonsense\r\n";
is(scalar <$sock>, "STORED\r\n", "stored barval");

mem_get_is($sock, "moo", "help_me_I_am_tired_of_this_nonsense");

mem_get_len_is($sock, "moo", 1, "h");

mem_get_len_is($sock, "moo", 4, "help");

mem_get_len_is($sock, "moo", 10, "help_me_I_");

mem_get_len_is($sock, "moo", 33, "help_me_I_am_tired_of_this_nonsen");

mem_get_len_is($sock, "moo", 34, "help_me_I_am_tired_of_this_nonsens");

mem_get_len_is($sock, "moo", 35, "help_me_I_am_tired_of_this_nonsense");

mem_get_len_is($sock, "moo", 100, "help_me_I_am_tired_of_this_nonsense");

@result = mem_gets($sock, "moo");
mem_gets_len_is($sock, $result[0], "moo", 10, "help_me_I_");

@result = mem_gets($sock, "moo");
mem_gets_len_is($sock, $result[0], "moo", 100, "help_me_I_am_tired_of_this_nonsense");

# cas success
print $sock "cas moo 0 0 31 $result[0]\r\nwhatever_so_what_you_changed_me\r\n";
is(scalar <$sock>, "STORED\r\n", "cas success, set moo");

# cas failure (reusing the same key)
print $sock "cas moo 0 0 5 $result[0]\r\nNEVER\r\n";
is(scalar <$sock>, "EXISTS\r\n", "cas failure, reusing a CAS ID");

@result = mem_gets($sock, "moo");
mem_gets_len_is($sock, $result[0], "moo", 10, "whatever_s");

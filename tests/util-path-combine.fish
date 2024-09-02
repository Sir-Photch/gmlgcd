#!/usr/bin/env fish

set builddir "$(status dirname)/../builddir"

function test_combine
    set -l actual   "$(echo $argv | eval "$builddir/test_util combine")"
    set -l expected "$(string join '/' (string trim -rc '/' $argv[1]) (string trim -lc '/' $argv[2]))"

    if test "$actual" != "$expected"
        echo "actual: $actual | expected: $expected" 1>&2
        exit 1
    end
end

test_combine "$HOME" mydir/mypath
test_combine "$HOME/" "/mydir"
test_combine "$HOME/" "mydir/"
test_combine "asdf/" "/jkloe"
test_combine "" "hehe"
test_combine "" "/hehe"
test_combine "/hehe/" ""

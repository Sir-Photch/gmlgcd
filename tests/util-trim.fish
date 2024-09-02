#!/usr/bin/env fish

set builddir "$(status dirname)/../builddir"

function test_trim 
    set -l actual   "$(echo $argv[1] | eval "$builddir/test_util trim")"
    set -l expected "$(string trim -lr $argv[1])"

    if test $expected != $actual
        echo "expected: $expected | actual: $actual" 1>&2
        exit 1
    end
end

function randstr
    set -l rand "$(cat /dev/urandom | tr -dc '[:print:]' | head -c (random 0 512))"

    for _i in seq 10
        if test "$(math "$(random) % 2")" = 1
            set -l rand " $rand"
        end
        if test "$(math "$(random) % 2")" = 1
            set -l rand "$rand "
        end
    end

    echo $rand
end

test_trim 'hello world'
test_trim ' asdf jkloe ö'
test_trim '     __!24_ '
test_trim '11123 123123  444   '

for _i in seq 1000
    test_trim "$(randstr)"
end

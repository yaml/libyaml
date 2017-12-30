#!/bin/bash

test=$1
type=$2

testfiles=tests/run-test-suite/data/[A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9]
testdir=tests/run-test-suite/list

testline() {
    echo `basename $1`": "`cat $1/===`
}

if [[ "$test" == parser ]]; then

    if [[ "$type" == valid ]]; then

        for i in $testfiles; do
            [[ -f $i/error ]] || echo "$(testline $i)"
        done > $testdir/libyaml-parser.list

    else
        for i in $testfiles; do
            [[ -f $i/error ]] && echo "$(testline $i)"
        done > $testdir/libyaml-parser-error.list
    fi


elif [[ "$test" == emitter ]]; then
    for i in $testfiles; do
        [[ -f $i/error ]] || echo "$(testline $i)"
    done > $testdir/libyaml-emitter.list
fi


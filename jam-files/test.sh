#!/bin/bash
g++ "$@" -x c++ - <<<'int main() {}' >/dev/null 2>/dev/null
echo -n $?

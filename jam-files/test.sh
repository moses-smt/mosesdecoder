#!/bin/bash
g++ "$@" -x c++ - <<<'int main() {}' -o /dev/null >/dev/null 2>/dev/null
echo -n $?

#!/bin/sh

set -x

find . -type f -name '*.log' -exec rm {} +
# find . -type f -name '*.log' -exec rm -i {} +
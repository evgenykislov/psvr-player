#!/bin/bash

rm -rfd glad
echo "Run glad generator"
glad --out-path=glad --generator=c --no-loader


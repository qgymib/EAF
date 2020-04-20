#!/bin/bash

# coverage should only valid on master branch
if [ "$TRAVIS_BRANCH" != "master" ]; then
	exit 0
fi

if [ "$CC" = "gcc" ]; then
	bash <(curl -s https://codecov.io/bash)
fi

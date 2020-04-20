#!/bin/bash

# coverage should only valid on master branch
if [ "$TRAVIS_BRANCH" != "master" ]; then
	exit 0
fi

# upload coverage
bash <(curl -s https://codecov.io/bash)

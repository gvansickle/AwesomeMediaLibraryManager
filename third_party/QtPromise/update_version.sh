#!/bin/bash

EXIT_OK=0
EXIT_USAGE=64

usage()
{
	SCRIPTNAME=$(basename "$0") 
	echo "Usage: $SCRIPTNAME [--help|-h] NEWVERSION"
}

case "$1" in
	"" )
		echo "Missing required parameter: NEWVERSION" >&2
		usage
		exit $EXIT_USAGE
		;;
	"--help" | "-h" )
		usage
		exit $EXIT_OK
		;;
esac

NEWVERSION=${1}

TODAY=$(date -I)
THIS_YEAR=$(date +%Y)
VERSION_REGEX='[0-9]+\.[0-9]+\.[0-9]+'
sed -i -E -e "s/version: \"$VERSION_REGEX/version: \"$NEWVERSION/" .appveyor.yml
sed -i -E -e "s/set\\(PROJECT_VERSION $VERSION_REGEX/set(PROJECT_VERSION $NEWVERSION/" CMakeLists.txt
sed -i -E -e "s/## \\[Unreleased\\] ##/## [$NEWVERSION] - $TODAY ##/" CHANGELOG.md
echo "[$NEWVERSION]: https://gitlab.com/julrich/QtPromise/tags/$NEWVERSION" >>CHANGELOG.md
sed -i -E -e "s/Copyright (\\(c\\)|©) [0-9]+/Copyright \\1 $THIS_YEAR/" LICENSE README.md
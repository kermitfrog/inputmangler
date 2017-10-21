#!/bin/zsh
# $1 is the source root

cd $1/doc
gzip -c README > release/README.gz
gzip -c LICENSE > release/LICENSE.gz
gzip -c config.xml.example > release/config.xml.example.gz
cp changelog release/


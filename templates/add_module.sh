#!/bin/sh

set -eu

module=$1
reporoot="$(git rev-parse --show-toplevel)"

upper_case() {
	echo "$1" | tr '[:lower:]' '[:upper:]'
}

isubstitution=s,mod_template\\.h,modules/$module.h,
substitution=s/mod_template/$module/g
Substitution=s/$(upper_case mod_template)/$(upper_case $module)/g

sed -e $isubstitution -e $substitution -e $Substitution $reporoot/templates/mod_template.cc > $reporoot/modules/$module.cc
sed -e $substitution -e $Substitution $reporoot/templates/mod_template.h > $reporoot/include/modules/$module.h

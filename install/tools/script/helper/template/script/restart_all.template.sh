#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )";
cd "$SCRIPT_DIR";

source common/common.sh

sh stop_all.sh $* ;


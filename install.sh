#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
sudo ln -s $DIR/include/ /usr/local/include/NoiseGenerator

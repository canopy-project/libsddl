#!/bin/bash
rm -r libsddl_0.9.tar.gz
rm -rf libsddl_0.9

mkdir libsddl_0.9
cp -r include libsddl_0.9
cp -r src libsddl_0.9
cp *.txt makefile libsddl_0.9
tar -cvzf libsddl_0.9.tar.gz libsddl_0.9

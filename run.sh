#!/bin/sh

# コンパイル
make clean
make

# 順に実行
ip = hostname -i
./server $ip $ip
./member
./db


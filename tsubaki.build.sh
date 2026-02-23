#!/bin/bash
sudo g++ -std=c++17 -O3 ./Tsubaki.Qt_creator.d/main.cpp -o /usr/bin/tsubaki -lssl -lcrypto -lpthread
#这里列出了建议的编译命令。当然你可以自己选择其他的参数，或者换用其他的编译器。
#The suggested compilation commands are listed here. Of course, you can choose other parameters, or change to other compilers by yourself.

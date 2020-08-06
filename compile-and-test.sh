#!/bin/sh
c++ -std=c++17 -Wall -Wextra -fsanitize=undefined -g -fno-omit-frame-pointer -fsanitize=undefined test.cpp && ./a.out

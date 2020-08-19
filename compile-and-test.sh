#!/bin/sh
c++ -std=c++17 -Wall -Wextra -fsanitize=undefined -g -fno-omit-frame-pointer test.cpp && ./a.out

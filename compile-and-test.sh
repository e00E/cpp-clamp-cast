#!/bin/sh
c++ -std=c++17 -Werror -Wall -Wextra -Wconversion -fsanitize=undefined -g -fno-omit-frame-pointer test.cpp && ./a.out

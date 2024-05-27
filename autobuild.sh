#!/bin/bash

# 如果出现错误立即停止执行
set -e

# 删除当前目录build下的所有文件
rm -rf `pwd`/build/*

# 进入build文件目录进行编译
cd `pwd`/build &&
    cmake .. &&
    make

cd ..

cp -r `pwd`/src/include `pwd`/lib
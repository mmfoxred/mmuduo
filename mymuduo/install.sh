#!/bin/bash
set -e

#判断并新建build目录
if [ ! -d `pwd`/build ]; then #这是Esc下的符号，不是单引号
    mkdir `pwd`/build
fi

#清理build
rm -rf `pwd`/build/*

#编译
make clean && make

#安装 头文件 so文件
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi
if [ ! -d /usr/lib/mymuduo ]; then 
    mkdir /usr/lib/mymuduo
fi

#头文件
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

#so库文件
cp `pwd`/build/libmymuduo.so /usr/lib/mymuduo/

#刷新缓存
export LD_LIBRARY_PATH=/usr/lib/mymuduo:$LD_LIBRARY_PATH
sudo ldconfig -v

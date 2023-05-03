#!/bin/bash
set -e

#判断并新建build目录
if [! -d 'pwd'/build]; then
    mkdir 'pwd'/build
fi

#清理build
rm -rf 'pwd'/build/*

#编译
cd 'pwd'/build && make ..

#安装 头文件 so文件
cd ..
if [! -d /usr/include/mymuduo]; then
    mkdir /usr/include/mymuduo
fi

#头文件
for header in 'ls *.h'
do
    cp $header /usr/include/mymuduo
done

#so库文件
cp 'pwd'/build/*.so /usr/lib

#刷新缓存
ldconfig
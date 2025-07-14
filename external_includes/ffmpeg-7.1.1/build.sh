basepath=$(cd `dirname $0`;pwd)

echo ${basepath}


cd ${basepath}

./configure --prefix=${basepath}/ffmpeg_install --disable-static --enable-shared  --disable-asm --enable-debug=3

make -j4
make install

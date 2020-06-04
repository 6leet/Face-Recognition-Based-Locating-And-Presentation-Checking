# lab_project

pkg-config --cflags --libs /usr/local/Cellar/opencv@3/3.4.10_3/lib/pkgconfig/opencv.pc

export PKG_CONFIG_PATH="/usr/local/Cellar/opencv@3/3.4.10_3/lib/pkgconfig"

g++ $(pkg-config --cflags --libs opencv) -std=c++11  reposition.cpp -o reposition

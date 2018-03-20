CC		= arm-linux-gnueabi-gcc #arm-none-linux-gnueabi-g++
CXX		= arm-linux-gnueabi-g++ #arm-none-linux-gnueabi-g++

all:RY-krec

VIDEO_LIB = -L./video/lib -lmp4v2 -lvpu -lpthread -ljpeg
VIDEO_INC = -I./video -I./video/include
VIDEO_SRC = ./video/cam_cap.cpp \
            ./video/rgb_bmp.cpp \
            ./video/yuv2rgb.cpp \
            ./video/VPUWraper.cpp \
            ./video/vpucls.cpp \
            ./video/cam_h264.cpp \
            ./video/toollib.cpp \
            ./video/frame_mng.cpp \
            ./video/yuvaddtime.cpp \
            ./video/yuvadd.cpp \
            ./video/video_mng.cpp
VIDEO_OBJ = ./video/cam_cap.o \
            ./video/rgb_bmp.o \
            ./video/yuv2rgb.o \
            ./video/VPUWraper.o \
            ./video/vpucls.o \
            ./video/cam_h264.o \
            ./video/toollib.o \
            ./video/frame_mng.o \
            ./video/yuvaddtime.o \
            ./video/yuvadd.o \
            ./video/video_mng.o

MZMQ_LIB =
MZMQ_INC =
MZMQ_SRC =
MZMQ_OBJ =

OPENCV_LIB = -L./video/opencv/lib -lopencv_core
OPENCV_INC = -I./video/opencv/include
OPENCV_SRC =
OPENCV_OBJ =

SND_LIB = #-L./snd/lib -lfaac -lasound
SND_INC = -I./snd -I./snd/inc
SND_SRC =
SND_OBJ =

SQMNG_LIB = -L./
SQMNG_INC = -I./
SQMNG_SRC =
SQMNG_OBJ =


ADXL_LIB = -L./
ADXL_INC = -I./
ADXL_SRC =
ADXL_OBJ =

LDFLAGS += $(VIDEO_LIB)  $(MZMQ_LIB) $(SND_LIB) $(OPENCV_LIB) -lpthread 


INCLUDE = $(VIDEO_INC) $(MZMQ_INC) $(SND_INC) $(OPENCV_INC) -I./


#SRC = $(VIDEO_SRC) $(MZMQ_SRC) $(SND_SRC) main.cpp msg_mng.cpp 
#-include $(SRC:.cpp=.d)
#%.d:%.cpp
#	@set -e; rm -f $@;\
#	$(CC) $(INCLUDE) -MM $< > $@.$$$$;\
#	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
#	rm -f $@.$$$$

%.o:%.cpp
	$(CXX) -std=c++11 -c -g -o $@ $< $(INCLUDE) -Wno-write-strings

MACRO = -D SQLITE_HAS_CODEC
sqlite3.o:
	$(CC) -c -o sqlite3.o sqlite3.c -lpthread -ldl $(MACRO)

RY-krec:main.o $(ADXL_OBJ) $(SQMNG_OBJ) $(VIDEO_OBJ) $(MZMQ_OBJ) $(SND_OBJ)
	$(CXX) -std=c++11 -o $@ $^ $(LDFLAGS) -ldl

clean:
	rm -rf RY-krec *~ *.o *.d *.d.*
	cd video; rm -rf RY-krec *~ *.o *.d *.d.*
	cd m_zmq; rm -rf RY-krec *~ *.o *.d *.d.*
	cd snd; rm -rf RY-krec *~ *.o *.d *.d.*

cp:
	cp RY-krec /home/rootfs/


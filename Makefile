#arg: arg.c
#	arm-linux-gnueabi-gcc -o arg arg.c
#install:	
#	cp hello	/opt/rootfs/root/
#clean:	
#	rm -f hello /opt/rootfs/root/hello

CC		= gcc
DEST		= /usr/local/bin/
LIBS		= -lfreetype -ljpeg -lts -lasound -lpafe -lmysqlclient
PROGRAM		= myRaspiApp
OBJS		= $(PROGRAM).o ./func/font.o ./func/utf8to32.o ./func/jpeg.o ./func/bitmap.o ./func/debug.o ./func/os_wrapper.o ./p1/p1.o ./p2/p2.o ./p3/p3.o ./p3/p3_mysql.o ./p3/p3_cmn.o ./p4/p4.o
CFLAGS		=  -I./ -I/usr/include/freetype2 -I./func -I./p1 -I./p2 -I./p3 -I./p4 -I/usr/local/include/libpafe -I./cmn -I/usr/include/mysql
INCLUDE   	= 

$(PROGRAM):	$(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(PROGRAM)

install:
	sudo cp $(PROGRAM) $(DEST)
clean:
	rm -f ./func/*.o ./p1/*.o ./p2/*.o ./p3/*.o ./p4/*.o *.o $(PROGRAM)
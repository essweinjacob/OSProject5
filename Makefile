CC = gcc
HEADER = shared.h
CFLAGS = -g
TARGET1 = oss
TARGET2 = user
OBJS1 = oss.o
OBJS2 = user.o
.SUFFIXES: .c .o

all: oss user

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1)
$(TARGET2): $(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2)
.c .o: $(HEADER)
	$(CC) $(CFLAGS) -c $<
clean:
	/bin/rm -f *.o $(TARGET1)

CC			= g++
CFLAGS		= -Wall 
INCLUDES    = -Isrc
LIBS		= -lsqlite3
OBJS		= sqlite3_serializer.o
TARGET		= librecapcore.so
TEST_TARGET = core-tester

ifeq ($(build),debug) 
	CFLAGS += -ggdb -O1
else
	CFLAGS += -O2
endif

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -shared -o$@ $^ $(LIBS)

sqlite3_serializer.o:src/sqlite3_serializer.cpp
	$(CC) $(INCLUDES) -c $(CFLAGS) -fPIC -o$@ $<

clean:
	rm -f $(OBJS)

distclean:clean
	rm -f $(TARGET) $(TEST_TARGET)

test:$(TARGET) src/tester.cpp
	$(CC) $(INCLUDES) $(CFLAGS) src/tester.cpp -o$(TEST_TARGET) -L./ -lrecapcore

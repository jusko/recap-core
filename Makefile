CC			= g++
CFLAGS		= -Wall 
LIBS		= -lsqlite3
OBJS		= sqlite3_serializer.o
TARGET		= librecapcore.so
TEST_TARGET = recap_test

ifeq ($(build),debug) 
	CFLAGS += -ggdb -O1
else
	CFLAGS += -O2
endif

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -shared -o$@ $^ $(LIBS)

sqlite3_serializer.o:sqlite3_serializer.cpp
	$(CC) -c $(CFLAGS) -fPIC -o$@ $<

clean:
	rm -f $(OBJS)

distclean:clean
	rm -f $(TARGET) $(TEST_TARGET)

test:$(TARGET) tester.cpp
	$(CC) $(CFLAGS) tester.cpp -o$(TEST_TARGET) -L./ -lrecapcore

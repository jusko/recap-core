CC			= g++
CFLAGS		= -Wall `gpgme-config --cflags`
INCLUDES    = -Isrc
LIBS		= -lsqlite3 `gpgme-config --libs`
OBJS		= sqlite3_serializer.o gpgme_wrapper.o
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

gpgme_wrapper.o:src/gpgme_wrapper.cpp
	$(CC) $(INCLUDES) -c $(CFLAGS) -fPIC -o$@ $<

clean:
	rm -f $(OBJS)

distclean:clean
	rm -f $(TARGET) $(TEST_TARGET)

test:$(TARGET) src/tester.cpp
	$(CC) $(INCLUDES) $(CFLAGS) src/tester.cpp -o$(TEST_TARGET) -L./ -lrecapcore

regression_tests:$(TARGET) src/regression_tests.cpp
	$(CC) $(INCLUDES) $(CFLAGS) src/regression_tests.cpp -o regression_tests -L./ -lrecapcore

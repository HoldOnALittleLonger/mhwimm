all: mhwimm

vpath %.c src/
vpath %.cc src/
vpath %.h  inc/

CC = g++
CXXFLAGS = -g -std=gnu++2a -DDEBUG
OBJECTS = main.o mhwimm_ui.o mhwimm_executor.o mhwimm_database.o mhwimm_ui_thread.o mhwimm_executor_thread.o mhwimm_database_thread.o mhwimm_db_reg_routines.o mhwimm_sig_actions.o sqlite3.o
HEADERS_PATH = inc/

.SUFFIXES: .o
%.o: %.cc
	$(CC) $(CXXFLAGS) -o $@ -c $< -I$(HEADERS_PATH)

mhwimm: $(OBJECTS)
	$(CC) -o $@ $(CXXFLAGS) $^ -lpthread -ldl

sqlite3.o: sqlite3.c
	gcc -o $@ -c $<

.PHONY: clean
clean:
	rm -f *.o



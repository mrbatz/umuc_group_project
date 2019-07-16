CC=clang
CFLAGS=-Wall -Werror -I "/var/openssl-1.1.1a/include"
LDFLAGS=-L "/var/openssl-1.1.1a" -lssl -lcrypto
LIB_SOURCES=$(wildcard src/lib/*.c)
TASK_SOURCES=$(wildcard src/lib/tasks/*.c)
PACKAGE_SOURCES=$(wildcard src/lib/tasks/package_managers/*.c)

MAIN=src/main.c
SOURCES=$(LIB_SOURCES) $(TASK_SOURCES) $(PACKAGE_SOURCES) $(MAIN)
BUILDDIR=build
EXECUTABLE=agent.exe

all: build $(EXECUTABLE) link

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/$(EXECUTABLE) $(SOURCES) $(LDFLAGS)

build:
	@mkdir -p $(BUILDDIR)

debug: CFLAGS += -DDEBUG -g
debug: all
    
debugr: clean debug

link:
	ln -s $(BUILDDIR)/$(EXECUTABLE) $(EXECUTABLE)

r: clean all

clean:
	rm -rf $(BUILDDIR) $(EXECUTABLE);

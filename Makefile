CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRCS = src/logger.o src/config.o src/scan.o src/request.o src/main.o
TARGET = build/media

SYSTEMD_SERVICE = media.service
SYSTEMD_DIR = /etc/systemd/system

.PHONY: assets, build, start, install, uninstall, config

all: assets

$(TARGET): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^ 

assets: $(TARGET)
ifeq (,$(wildcard build/media.cnf))
	cp assets/media.cnf.example build/media.cnf
endif

src/main.o: src/scan.h src/main.c
	gcc -c src/main.c -o src/main.o -g

src/config.o: src/config.h src/config.c
	gcc -c src/config.c -o src/config.o -g

src/scan.o: src/scan.h src/scan.c
	gcc -c src/scan.c -o src/scan.o -g

src/request.o: src/request.h src/request.c
	gcc -c src/request.c -o src/request.o -g

src/logger.o: src/logger.h src/logger.c
	gcc -c src/logger.c -o src/logger.o -g
	
start:
	.$(TARGET) 

config:
	mkdir -p build
	sed s/%user%/${USER}/ assets/${SYSTEMD_SERVICE} > build/${SYSTEMD_SERVICE}
	
install: $(TARGET)
	mkdir -p /var/log/media
	chmod 777 /var/log/media
	mkdir -p /etc/media
	chmod 755 /etc/media
	install -m 755 assets/media.cnf.example /etc/media/media.cnf
	install -m 755 $(TARGET) /usr/local/bin
	install -m 644 build/$(SYSTEMD_SERVICE) $(SYSTEMD_DIR)

	sudo systemctl daemon-reload

	sudo systemctl enable media
	sudo systemctl start media

uninstall:
	# Stop and disable the service
	sudo systemctl stop media || true
	sudo systemctl disable media || true

	# Remove the executable and systemd service file
	sudo rm -f /usr/local/bin/media
	sudo rm -f $(SYSTEMD_DIR)/$(SYSTEMD_SERVICE)

	# Reload systemd after removing the service file
	sudo systemctl daemon-reload


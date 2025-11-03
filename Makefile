CC=g++
CFLAGS=-Wall -O2 -std=c++17 -Iincludes
LIBS=-lboost_system -lcurl -lpthread
TARGET=smtp2telegram
SRC=src/smtp2telegram.cpp src/Config.cpp src/Logger.cpp src/TelegramClient.cpp src/EmailParser.cpp src/SMTPServer.cpp
BUILDDIR=build
DEBDIR=$(BUILDDIR)/debian/$(TARGET)
BINTARGET=$(BUILDDIR)/$(TARGET)
VERSION=2.0.0
ARCH=$(shell dpkg-architecture -qDEB_BUILD_ARCH)

all: $(BUILDDIR) $(BINTARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINTARGET): $(SRC) includes/smtp2telegram.h includes/Config.h includes/Logger.h includes/TelegramClient.h includes/EmailParser.h includes/SMTPServer.h
	$(CC) $(CFLAGS) $(SRC) -o $(BINTARGET) $(LIBS)

clean:
	rm -rf $(BUILDDIR)
	rm -f $(TARGET)_$(VERSION)_$(ARCH).deb

deb: all install-service
	mkdir -p $(DEBDIR)/usr/bin
	cp $(BINTARGET) $(DEBDIR)/usr/bin/
	mkdir -p $(DEBDIR)/DEBIAN
	echo "Package: $(TARGET)" > $(DEBDIR)/DEBIAN/control
	echo "Version: $(VERSION)" >> $(DEBDIR)/DEBIAN/control
	echo "Section: base" >> $(DEBDIR)/DEBIAN/control
	echo "Priority: optional" >> $(DEBDIR)/DEBIAN/control
	echo "Architecture: $(ARCH)" >> $(DEBDIR)/DEBIAN/control
	echo "Maintainer: William Bellavance <william@bellavance.co>" >> $(DEBDIR)/DEBIAN/control
	echo "Description: smtp2telegram service" >> $(DEBDIR)/DEBIAN/control

	# Post install script
	echo "#!/bin/bash" > $(DEBDIR)/DEBIAN/postinst
	echo "systemctl daemon-reexec" >> $(DEBDIR)/DEBIAN/postinst
	echo "systemctl daemon-reload" >> $(DEBDIR)/DEBIAN/postinst
	echo "systemctl enable $(TARGET).service" >> $(DEBDIR)/DEBIAN/postinst
	chmod +x $(DEBDIR)/DEBIAN/postinst
	echo 'echo "[smtp2telegram] NOTE: If you have a .env file in your home directory, make sure it contains LOG_KEEP_DAYS=3 (or your preferred value) for log retention."' >> $(DEBDIR)/DEBIAN/postinst

	# Pre-removal script
	echo "#!/bin/bash" > $(DEBDIR)/DEBIAN/prerm
	echo "systemctl stop $(TARGET).service" >> $(DEBDIR)/DEBIAN/prerm
	echo "systemctl disable $(TARGET).service" >> $(DEBDIR)/DEBIAN/prerm
	chmod +x $(DEBDIR)/DEBIAN/prerm

	 # Add license
	 mkdir -p $(DEBDIR)/usr/share/doc/$(TARGET)
	 cp LICENSE $(DEBDIR)/usr/share/doc/$(TARGET)/

	 dpkg-deb --build $(DEBDIR) $(TARGET)_$(VERSION)_$(ARCH).deb

install-service:
	mkdir -p $(DEBDIR)/lib/systemd/system
	echo "[Unit]" > $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "Description=SMTP to Telegram Service" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "After=network.target" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "[Service]" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "ExecStart=/usr/bin/$(TARGET)" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "Restart=always" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "User=$$(if [ -n "$$SUDO_USER" ]; then echo $$SUDO_USER; else echo $$USER; fi)" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "[Install]" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service
	echo "WantedBy=multi-user.target" >> $(DEBDIR)/lib/systemd/system/$(TARGET).service

#include "Particle.h"

#include "SpiffsParticleRK.h"

// Pick a debug level from one of these two:
// SerialLogHandler logHandler;
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Chose a flash configuration:
// SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
// SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins), typical config for E series
SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module

// Create an object for the SPIFFS file system
SpiffsParticle fs(spiFlash);

void setup() {
	Serial.begin();

	// Wait until USB serial is connected, or 10 seconds
	waitFor(Serial.isConnected, 10000);

	spiFlash.begin();

	fs.withPhysicalSize(64 * 1024);

	s32_t res = fs.mountAndFormatIfNecessary();
	Log.info("mount res=%d", res);

	if (res == SPIFFS_OK) {
		// File system was mounted
		spiffs_DIR dir;
		spiffs_dirent dirent;
		int lastFileNum = 0;

		fs.opendir(NULL, &dir);

		while(fs.readdir(&dir, &dirent)) {
			Log.info("name=%s size=%lu", dirent.name, dirent.size);

			int fileNum = 0;
			if (sscanf((const char *)dirent.name, "test-%04d", &fileNum) == 1) {
				if (fileNum > lastFileNum) {
					lastFileNum = fileNum;
				}
			}
		}
		fs.closedir(&dir);

		char newName[32];
		snprintf(newName, sizeof(newName), "test-%04d", lastFileNum + 1);

		SpiffsParticleFile f = fs.openFile(newName, SPIFFS_O_RDWR|SPIFFS_O_CREAT);
		if (f.isValid()) {
			f.println("hello world");

			f.close();
		}
		Log.info("created %s", newName);
	}
}

void loop() {

}

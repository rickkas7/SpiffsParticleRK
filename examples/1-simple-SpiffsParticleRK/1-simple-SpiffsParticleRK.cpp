#include "Particle.h"

#include "SpiffsParticleRK.h"

// Pick a debug level from one of these two:
// SerialLogHandler logHandler;
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Chose a flash configuration:
SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
// SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins), typical config for E series
// SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module

// Create an object for the SPIFFS file system
SpiffsParticle fs(spiFlash);

void setup() {
	Serial.begin();

	spiFlash.begin();

	fs.withPhysicalSize(64 * 1024);

	s32_t res = fs.mountAndFormatIfNecessary();
	Log.info("mount res=%d", res);

	if (res == SPIFFS_OK) {
		SpiffsParticleFile f = fs.openFile("test", SPIFFS_O_RDWR|SPIFFS_O_CREAT);
		if (f.isValid()) {
			f.println("hello world");

			f.seekStart();

			String s = f.readStringUntil('\n');
			Log.info("got: %s", s.c_str());

			f.close();
		}
	}
}

void loop() {

}

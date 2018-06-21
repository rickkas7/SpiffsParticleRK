#include "Particle.h"

#include "SpiffsParticleRK.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

/**
 * This example is mainly
 */

// Pick a debug level from one of these two:
// SerialLogHandler logHandler;
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Chose a flash configuration:
// SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins)
// SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module

// Create an object for the SPIFFS file system
SpiffsParticle fs(spiFlash);

const unsigned long SLEEP_PERIOD_SEC = 60;
const unsigned long WAKE_PERIOD_MS = 30000;
const unsigned long WRITE_PERIOD_MS = 10000;
const int BOOT_MAGIC = 0x423e5cd6;

unsigned long lastSleepMs = 0;
unsigned long lastWriteMs = 0;
int counter = 0;

retained int bootValue = 0;

SpiffsParticleFile testFile;

void setup() {
	Serial.begin();

	spiFlash.begin();

	fs.withPhysicalSize(256 * 1024);

	s32_t res = fs.mountAndFormatIfNecessary();
	Log.info("mount res=%ld", res);

	if (res == SPIFFS_OK) {
		testFile = fs.openFile("test", SPIFFS_O_RDWR|SPIFFS_O_CREAT|SPIFFS_O_APPEND);
	}
}

void loop() {
	if (millis() - lastWriteMs >= WRITE_PERIOD_MS) {
		lastWriteMs = millis();

		if (testFile.isValid()) {
			char buf[256];
			snprintf(buf, sizeof(buf), "counter=%d millis=%lu time=%s", counter++, millis(), Time.timeStr().c_str());

			// Append to file
			testFile.println(buf);

			Log.info("wrote to file: %s", buf);
		}
	}
	if (millis() - lastSleepMs >= WAKE_PERIOD_MS) {
		lastSleepMs = millis();

		// Write any outstanding data on all open files, close the files, and unmount the volume before sleep.
		fs.unmount();

		// Put the Macronix flash in deep power down mode
		spiFlash.deepPowerDown();

		if (bootValue != BOOT_MAGIC) {
			bootValue = BOOT_MAGIC;

			// On cold boot, it's necessary to turn cellular on, otherwise the modem will not go into
			// full SLEEP_MODE_DEEP.
			// Note: The technique here only works without system thread enabled, see:
			// https://community.particle.io/t/electron-sleep-mode-deep-tips-and-examples/27823
			Cellular.on();

			Cellular.off();

			delay(5000);
		}


		System.sleep(SLEEP_MODE_DEEP, SLEEP_PERIOD_SEC);
	}
}



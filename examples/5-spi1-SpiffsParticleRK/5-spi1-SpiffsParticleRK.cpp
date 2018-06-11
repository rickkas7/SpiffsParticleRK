#include "Particle.h"

#include "SpiffsParticleRK.h"
#include "TestSuiteImpl.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler;
// SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// This uses SPI1 on the D pins:
// CS    D5  White
// SCK   D4  Orange
// MISO  D3  Blue  (DO)
// MOSI  D2  Green (DI)
SpiFlash spiFlash(SPI1, D5);
SpiffsParticle fs(spiFlash);


unsigned long delayTime = 4000;

void setup() {
	Serial.begin();

	spiFlash.begin();

	// Use 128K for now
	fs.withPhysicalSize(128 * 1024); //.withLowLevelDebug();

}

void loop() {
	delay(delayTime);
	delayTime = 60000;

	Log.info("chip id: %06x", spiFlash.jedecIdRead());
	if (!spiFlash.isValid()) {
		Log.info("not valid, skipping test");
		return;
	}

	Log.info("about to run test suite");
	runFullTestSuite();
}



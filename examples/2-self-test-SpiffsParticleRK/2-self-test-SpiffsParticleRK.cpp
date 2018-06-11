#include "Particle.h"

#include "SpiffsParticleRK.h"
#include "TestSuiteImpl.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);
SpiFlashISSI spiFlash(SPI, A2);
SpiffsParticle fs(spiFlash);
unsigned long delayTime = 4000;

void setup() {
	Serial.begin();

	spiFlash.begin();

	// Use 128K for now
	fs.withPhysicalSize(128 * 1024);

}

void loop() {
	delay(delayTime);
	delayTime = 60000;

	Log.info("chip id: %06lx", spiFlash.jedecIdRead());
	if (!spiFlash.isValid()) {
		Log.info("not valid, skipping test");
		return;
	}


	runFullTestSuite();
}


#include "Particle.h"

#include "SpiffsParticleRK.h"

SerialLogHandler logHandler; // (LOG_LEVEL_TRACE);
SpiFlashISSI spiFlash(SPI, A2);
SpiffsParticle fs(spiFlash);

void setup() {
	Serial.begin();

	delay(4000);

	spiFlash.begin();

	fs.withPhysicalSize(64 * 1024);

	s32_t res = fs.mount(NULL);
	Log.info("mount res=%d", res);

	if (res == SPIFFS_ERR_NOT_A_FS) {
		res = fs.format();
		Log.info("format res=%d", res);

		if (res == SPIFFS_OK) {
			res = fs.mount(NULL);
			Log.info("mount after format res=%d", res);
		}
	}

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

#include "Particle.h"

#include "SpiffsParticleRK.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler(LOG_LEVEL_WARN, { // Logging level for non-application messages
    { "app", LOG_LEVEL_INFO }, // Default logging level for all application messages
    { "app.spiffs", LOG_LEVEL_WARN } // Disable spiffs info and trace messages
});

// Chose a flash configuration:
// SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
// SpiFlashMacronix spiFlash(SPI, A2);	// Macronix flash on SPI (A pins)
// SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins), typical config for E series
SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module


// Create an object for the SPIFFS file system
SpiffsParticle fs(spiFlash);

const unsigned long TEST_PERIOD_MS = 120000;
unsigned long lastTestRun = 10000 - TEST_PERIOD_MS;

const size_t NUM_BLOCKS = 512;
uint8_t testBuf1[512], testBuf2[512];

const size_t APPEND_TEST_SIZE = 100;
const size_t APPEND_TEST_COUNT = 5000;

void fillBuf(uint8_t *buf, size_t size);

class LogTime {
public:
	inline LogTime(const char *desc) : desc(desc), start(millis()) {
		Log.info("starting %s", desc);
	}
	inline ~LogTime() {
		Log.info("finished %s: %lu ms", desc, millis() - start);
	}

	const char *desc;
	unsigned long start;
};


void setup() {
	Serial.begin();

	spiFlash.begin();


}

void loop() {
	if (millis() - lastTestRun >= TEST_PERIOD_MS) {
		lastTestRun = millis();

		// Set up a 1024KB (1 MB) volume
		fs.withPhysicalSize(1024 * 1024);

		s32_t res;

		{
			LogTime time("chipErase");
			spiFlash.chipErase();
		}

#if 0
		{
			LogTime time("erase flash");

			res = fs.erase();
			Log.info("erase res=%ld", res);
		}
#endif

		// Need to call mount once before format
		res = fs.mount();

		{
			LogTime time("format");

			res = fs.format();
			Log.info("format res=%ld", res);
		}

		{
			LogTime time("mount");

			res = fs.mount();
			Log.info("mount res=%ld", res);
		}

		if (res == SPIFFS_OK) {
			Log.info("testing %u bytes in %u byte blocks ", NUM_BLOCKS * sizeof(testBuf1), sizeof(testBuf1));

			SpiffsParticleFile f = fs.openFile("test", SPIFFS_O_RDWR|SPIFFS_O_CREAT);
			if (f.isValid()) {
				LogTime time("write");

				srand(0);

				for(size_t blocks = 0; blocks < NUM_BLOCKS; blocks++) {
					fillBuf(testBuf1, sizeof(testBuf1));
					f.write(testBuf1,  sizeof(testBuf1));
				}

				f.close();
			}

			f = fs.openFile("test", SPIFFS_O_RDWR);
			if (f.isValid()) {
				LogTime time("read");

				srand(0);

				bool matching = true;

				for(size_t block = 0; block < NUM_BLOCKS; block++) {
					fillBuf(testBuf1, sizeof(testBuf1));
					f.readBytes((char *)testBuf2,  sizeof(testBuf2));

					if (matching) {
						for(size_t ii = 0; ii < sizeof(testBuf1); ii++) {
							if (testBuf1[ii] != testBuf2[ii]) {
								Log.info("mismatched data block=%u ii=%u expected=%02x got=%02x (ignoring remaining errors)", block, ii, testBuf1[ii], testBuf2[ii]);
								matching = false;
							}
						}
					}
				}

				f.close();
			}

			fs.remove("test");

			Log.info("testing append and flush %u bytes %u times ", APPEND_TEST_SIZE, APPEND_TEST_COUNT);

			f = fs.openFile("test2", SPIFFS_O_RDWR|SPIFFS_O_CREAT|SPIFFS_O_APPEND);
			if (f.isValid()) {
				LogTime time("append");

				srand(0);

				for(size_t blocks = 0; blocks < NUM_BLOCKS; blocks++) {
					fillBuf(testBuf1, APPEND_TEST_SIZE);
					f.write(testBuf1, APPEND_TEST_SIZE);
					f.flush();
				}

				f.close();
			}

			f = fs.openFile("test2", SPIFFS_O_RDWR);
			if (f.isValid()) {
				LogTime time("read");

				srand(0);

				bool matching = true;

				for(size_t block = 0; block < NUM_BLOCKS; block++) {
					fillBuf(testBuf1, APPEND_TEST_SIZE);
					f.readBytes((char *)testBuf2,  APPEND_TEST_SIZE);

					if (matching) {
						for(size_t ii = 0; ii < APPEND_TEST_SIZE; ii++) {
							if (testBuf1[ii] != testBuf2[ii]) {
								Log.info("mismatched data block=%u ii=%u expected=%02x got=%02x (ignoring remaining errors)", block, ii, testBuf1[ii], testBuf2[ii]);
								matching = false;
							}
						}
					}
				}

				f.close();
			}


			fs.unmount();
		}

	}
}

void fillBuf(uint8_t *buf, size_t size) {
	for(size_t ii = 0; ii < size; ii += 4) {
		int val = rand();
		buf[ii] = (uint8_t) (val >> 24);
		buf[ii + 1] = (uint8_t) (val >> 16);
		buf[ii + 2] = (uint8_t) (val >> 8);
		buf[ii + 3] = (uint8_t) val;
	}
}


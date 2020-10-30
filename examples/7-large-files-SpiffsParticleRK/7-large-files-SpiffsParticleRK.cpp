
#include "Particle.h"

#include "SpiffsParticleRK.h"

// Test program that creates a number of large files for testing. 
// Used to exercise the whole flash over time.

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler(LOG_LEVEL_WARN, { // Logging level for non-application messages
    { "app", LOG_LEVEL_INFO }, // Default logging level for all application messages
    { "app.spiffs", LOG_LEVEL_WARN } // Disable spiffs info and trace messages
});

// Chose a flash configuration:
// SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
SpiFlashMacronix spiFlash(SPI, A2);	// Macronix flash on SPI (A pins)
// SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins), typical config for E series
// SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module

// Create an object for the SPIFFS file system
SpiffsParticle fs(spiFlash);

const unsigned long TEST_PERIOD_MS = 120000;
unsigned long lastTestRun = 10000 - TEST_PERIOD_MS;

typedef void (*StateHandler)();
StateHandler stateHandler = 0;

const size_t BLOCK_SIZE = 512; // This affects RAM usage!
uint8_t testBuf1[BLOCK_SIZE], testBuf2[BLOCK_SIZE];


// Number of BLOCK_SIZE bytes per file
// For BLOCK_SIZE = 512, NUM_BLOCKS = 512: files are 256 Kbytes each
const size_t NUM_BLOCKS = 512;

size_t blockNum = 0;


// Each file is 256 Kbytes = 1/4 Mbytes
// The maximum number of files was determined by seeing when writes fail because the
// volume was full. There is more overhead than I would have thought.

// W25Q32JV = 32 Mbit = 4 Mbyte: NUM_FILES = 12, PHYS_SIZE = 4 * 1024 * 1024
// 64 Mbit = 8 Mbyte: NUM_FILES = 24 PHYS_SIZE = 8 * 1024 * 1024
// 128 Mbit = 16 Mbyte: NUM_FILES = 39, PHYS_SIZE = 16 * 1024 * 1024 
// MX25L25645G = 256 Mbit = 32 Mbyte: NUM_FILES = 96, PHYS_SIZE = 32 * 1024 * 1024 <= does not work!
const size_t NUM_FILES = 54;
const size_t PHYS_SIZE = 16 * 1024 * 1024;


size_t fileNum = 0;
char fileName[12];
SpiffsParticleFile file;
unsigned long fileStartTime;

void fillBuf(uint8_t *buf, size_t size);

void stateWaitNextRun();
void stateChipEraseAndMount();
void stateStartNextFile();
void stateWriteAndTest();
void stateVerifyNextFile();
void stateVerifyBlocks();
void stateDeleteFiles();
void stateFailure();


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

    stateHandler = stateWaitNextRun;
}

void loop() {
    if (stateHandler) {
        (*stateHandler)();
    }
}


void stateWaitNextRun() {
	if (millis() - lastTestRun < TEST_PERIOD_MS) {
        return;
    }
	
    lastTestRun = millis();
    stateHandler = stateChipEraseAndMount;
}

void stateChipEraseAndMount() {
	Log.info("sending reset to flash chip");
	spiFlash.resetDevice();

	if (!spiFlash.isValid()) {
		Log.info("failed to detect flash chip");
		stateHandler = stateFailure;
		return;
	}

    fs.withPhysicalSize(PHYS_SIZE); // parameter is in bytes

	if (PHYS_SIZE > (16 * 1024 * 1024)) {
		// NOTE: This does not work. Even when setting the physicalSize and
		// logicalPageSize I can't write more than 16 Mbytes to file system.
		// Not sure why.

		// Larger than 16 Mbyte requires 32-bit addressing mode
		spiFlash.set4ByteAddressing(true);
		
		size_t logicalPageSize = PHYS_SIZE / 65536;
		fs.withLogicalPageSize(logicalPageSize);

		Log.info("enabling 4-byte addressing and logicalPageSize=%u", logicalPageSize);
	}

    s32_t res;

    {
        LogTime time("chipErase");
        spiFlash.chipErase();
    }

    // Need to call mount once before format
    res = fs.mount();

    {
        LogTime time("format");

        res = fs.format();
        Log.info("format res=%ld", res);
		if (res != 0) {
			stateHandler = stateFailure;
			return;
		}
    }

    {
        LogTime time("mount");

        res = fs.mount();
        Log.info("mount res=%ld", res);
		if (res != 0) {
			stateHandler = stateFailure;
			return;
		}
    }

	srand(0);
	fileNum = 0;

    stateHandler = stateStartNextFile;
}

void stateStartNextFile() {
    // Create a new numbered file
	if (++fileNum > NUM_FILES) {
		Log.info("writes completed, verifying files now");
		srand(0);
		fileNum = 0;
		lastTestRun = millis();
		stateHandler = stateVerifyNextFile;
		return;
	}

	snprintf(fileName, sizeof(fileName), "t%d", fileNum);

	file = fs.openFile(fileName, SPIFFS_O_RDWR|SPIFFS_O_CREAT);
	if (!file.isValid()) {
        Log.info("open failed %d", __LINE__);
		stateHandler = stateFailure;
		return;
	}

	blockNum = 0;
	stateHandler = stateWriteAndTest;
	fileStartTime = millis();

	Log.info("writing file %u", fileNum);
}

void stateWriteAndTest() {

	if (++blockNum > NUM_BLOCKS) {
		// Done with this file
		Log.info("file %u completed in %lu ms", fileNum, millis() - fileStartTime);
		file.close();
		stateHandler = stateStartNextFile;
		return;
	}

	fillBuf(testBuf1, sizeof(testBuf1));

	size_t offset = file.tell();

	size_t writeResult = file.write(testBuf1, sizeof(testBuf1));
	if (writeResult != sizeof(testBuf1)) {
		Log.info("write failure %u != %u at offset %u", writeResult, sizeof(testBuf1), offset);
		file.close();
		stateHandler = stateFailure;
		return;
	}

	file.lseek(offset, SEEK_SET);

	file.readBytes((char *)testBuf2,  sizeof(testBuf2));

	for(size_t ii = 0; ii < sizeof(testBuf1); ii++) {
		if (testBuf1[ii] != testBuf2[ii]) {
			Log.info("mismatched data blockNum=%u ii=%u expected=%02x got=%02x", 
				blockNum, ii, testBuf1[ii], testBuf2[ii]);
		}
	}

}

void stateVerifyNextFile() {
	if (++fileNum > NUM_FILES) {
		Log.info("tests completed!");
		stateHandler = stateDeleteFiles;
		return;
	}

	snprintf(fileName, sizeof(fileName), "t%d", fileNum);

	file = fs.openFile(fileName, SPIFFS_O_RDWR|SPIFFS_O_CREAT);
	if (!file.isValid()) {
        Log.info("open failed %d", __LINE__);
		stateHandler = stateFailure;
		return;
	}

	blockNum = 0;
	stateHandler = stateVerifyBlocks;
	fileStartTime = millis();

	Log.info("verifying file %u", fileNum);
}

void stateVerifyBlocks() {
	if (++blockNum > NUM_BLOCKS) {
		// Done with this file
		Log.info("file %u verified in %lu ms", fileNum, millis() - fileStartTime);
		file.close();
		stateHandler = stateVerifyNextFile;
		return;
	}

	fillBuf(testBuf1, sizeof(testBuf1));

	file.readBytes((char *)testBuf2,  sizeof(testBuf2));

	for(size_t ii = 0; ii < sizeof(testBuf1); ii++) {
		if (testBuf1[ii] != testBuf2[ii]) {
			Log.info("mismatched data blockNum=%u ii=%u expected=%02x got=%02x", 
				blockNum, ii, testBuf1[ii], testBuf2[ii]);
		}
	}
}

void stateDeleteFiles() {
	Log.info("deleting all files");

	for(size_t ii = 0; ii < NUM_FILES; ii++) {
		snprintf(fileName, sizeof(fileName), "t%d", ii);
		fs.remove(fileName);
	}

	Log.info("running tests again...");
	srand(0);
	fileNum = 0;
    stateHandler = stateStartNextFile;

}

void stateFailure() {
	static bool reported = false;
	if (!reported) {
		reported = true;
		Log.info("entered failure state, tests stopped");
	}
}


#if 0
	if (millis() - lastTestRun >= TEST_PERIOD_MS) {
		lastTestRun = millis();

		// Set up a 1024KB (1 MB) volume
		fs.withPhysicalSize(1024 * 1024);

		s32_t res;

		{
			LogTime time("chipErase");
			spiFlash.chipErase();
		}

/*
		{
			LogTime time("erase flash");

			res = fs.erase();
			Log.info("erase res=%ld", res);
		}
*/

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
#endif


void fillBuf(uint8_t *buf, size_t size) {
	for(size_t ii = 0; ii < size; ii += 4) {
		int val = rand();
		buf[ii] = (uint8_t) (val >> 24);
		buf[ii + 1] = (uint8_t) (val >> 16);
		buf[ii + 2] = (uint8_t) (val >> 8);
		buf[ii + 3] = (uint8_t) val;
	}
}


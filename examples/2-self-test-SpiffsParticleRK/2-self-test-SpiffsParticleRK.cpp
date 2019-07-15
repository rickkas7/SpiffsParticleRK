#include "Particle.h"

#include "SpiffsParticleRK.h"

SYSTEM_THREAD(ENABLED);

// Optionally use SEMI_AUTOMATIC since we don't need cellular or Wi-Fi to run these tests
// SYSTEM_MODE(SEMI_AUTOMATIC);

// Pick a debug level from one of these two:
SerialLogHandler logHandler;
// SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Chose a flash configuration:
// SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
// SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins), typical config for E series
SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashWinbond spiFlash(SPI1, D5);	// Winbond flash on SPI1 (D pins)
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module

SpiffsParticle fs(spiFlash);
unsigned long delayTime = 4000;

void runFullTestSuite();

void setup() {
	Serial.begin();

	spiFlash.begin();

	// Use 128K for now
	fs.withPhysicalSize(128 * 1024);

	// Optionally enable low-level debug
	// fs.withLowLevelDebug();

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


bool testSimple() {
	bool result = true;


	// Open a file
	spiffs_file fh = fs.open("t1.txt", SPIFFS_O_RDWR|SPIFFS_O_CREAT );
	if (fh < 0) {
		Log.error("failure line %d fh=%d", __LINE__, fh);
		return false;
	}

	fs.write(fh, "test", 4);

	fs.lseek(fh, 0, SPIFFS_SEEK_SET);

	char buf[10];

	s32_t res = fs.read(fh, buf, sizeof(buf));
	if (res != 4) {
		Log.error("failure line %d res=%ld", __LINE__, res);
		result = false;
	}

	if (result) {
		if (strncmp(buf, "test", 4) != 0) {
			buf[4] = 0;
			Log.error("failure line %d str=%s", __LINE__, buf);
			result = false;
		}
	}

	fs.close(fh);


	fh = fs.open("notfound", SPIFFS_O_RDONLY);
	if (fh != SPIFFS_ERR_NOT_FOUND) {
		fs.close(fh);

		Log.error("failure line %d fh=%d", __LINE__, fh);
		result = false;
	}


	if (result) {
		Log.info("testSimple passed");
	}

	return result;
}

bool testAppend() {
	bool result = true;

	// Open a file
	spiffs_file fh = fs.open("t2.txt", SPIFFS_O_RDWR|SPIFFS_O_CREAT );
	if (fh < 0) {
		Log.error("failure line %d fh=%d", __LINE__, fh);
		return false;
	}

	s32_t res = fs.write(fh, "abcd", 4);
	if (res != 4) {
		Log.error("failure line %d res=%ld", __LINE__, res);
		result = false;
	}

	fs.close(fh);

	fh = fs.open("t2.txt", SPIFFS_O_RDWR|SPIFFS_O_APPEND );
	if (fh < 0) {
		Log.error("failure line %d fh=%d", __LINE__, fh);
		return false;
	}


	fs.write(fh, "efgh", 4);

	spiffs_stat stat;
	res = fs.fstat(fh, &stat);
	if (res != SPIFFS_OK || stat.size != 8) {
		Log.error("failure line %d res=%ld size=%lu", __LINE__, res, stat.size);
		return false;
	}

	fs.close(fh);

	fh = fs.open("t2.txt", SPIFFS_O_RDONLY );
	if (fh < 0) {
		Log.error("failure line %d fh=%d", __LINE__, fh);
		return false;
	}

	char buf[10];

	res = fs.read(fh, buf, sizeof(buf));
	if (res != 8) {
		Log.error("failure line %d res=%ld", __LINE__, res);
		result = false;
	}

	if (result) {
		if (strncmp(buf, "abcdefgh", 8) != 0) {
			buf[4] = 0;
			Log.error("failure line %d str=%s", __LINE__, buf);
			result = false;
		}
	}

	fs.close(fh);

	if (result) {
		Log.info("testAppend passed");
	}

	return result;
}

bool testDir1() {
	bool result = true;

	spiffs_DIR dir, *dirRes;
	spiffs_dirent dirEnt[3], *dirEntRes;

	dirRes = fs.opendir("", &dir);
	if (dirRes == 0) {
		Log.error("failure line %d", __LINE__);
		return false;
	}

	dirEntRes = fs.readdir(&dir, &dirEnt[0]);
	if (dirEntRes == 0) {
		Log.error("failure line %d", __LINE__);
		result = false;
	}
	if (result) {
		dirEntRes = fs.readdir(&dir, &dirEnt[1]);
		if (dirEntRes == 0) {
			Log.error("failure line %d", __LINE__);
			result = false;
		}
	}
	if (result) {
		dirEntRes = fs.readdir(&dir, &dirEnt[2]);
		if (dirEntRes != 0) {
			Log.error("failure line %d", __LINE__);
			result = false;
		}
	}
	if (result) {
		if (strcmp((char *)dirEnt[0].name, "t1.txt") == 0 && strcmp((char *)dirEnt[1].name, "t2.txt") == 0) {
			// Good
		}
		else
		if (strcmp((char *)dirEnt[0].name, "t2.txt") == 0 && strcmp((char *)dirEnt[1].name, "t1.txt") == 0) {
			// Good reverse order
		}
		else {
			Log.error("failure line %d 1=%s 2=%s", __LINE__, (char *)dirEnt[0].name, (char *)dirEnt[1].name);
			result = false;
		}
	}

	fs.closedir(&dir);


	if (result) {
		Log.info("testDir1 passed");
	}

	return result;
}

bool testWiring() {
	bool result = true;

	SpiffsParticleFile f = fs.openFile("t3.txt", SPIFFS_O_RDWR);
	if (f.isValid()) {
		// Should not exist yet
		f.close();
		Log.error("failure line %d", __LINE__);
		result = false;
	}

	f = fs.openFile("t3.txt", SPIFFS_O_RDWR|SPIFFS_O_CREAT);
	if (!f.isValid()) {
		Log.error("failure line %d", __LINE__);
		result = false;
	}

	f.println("line1");

	f.printlnf("line%d", 2);

	double d = 3.3333;
	f.println(d);

	f.seekStart();

	String s = f.readStringUntil('\n');

	s32_t res;

	if (result) {
		if (strcmp(s.c_str(), "line1\r") != 0) {
			Log.error("failure line %d len=%d s=%s", __LINE__, s.length(), s.c_str());
			result = false;
		}
	}

	if (result) {
		s = f.readStringUntil('\n');
		if (strcmp(s.c_str(), "line2\r") != 0) {
			Log.error("failure line %d len=%d s=%s", __LINE__, s.length(), s.c_str());
			result = false;
		}
	}

	if (result) {
		s = f.readStringUntil('\n');
		if (strcmp(s.c_str(), "3.33\r") != 0) {
			Log.error("failure line %d len=%d s=%s", __LINE__, s.length(), s.c_str());
			result = false;
		}
	}

	if (result) {
		res = f.truncate(7);
		if (res != SPIFFS_OK) {
			Log.error("failure line %d res=%ld", __LINE__, res);
			result = false;
		}
	}

	if (result) {
		f.seekStart();

		if (f.available() != 7) {
			Log.error("failure line %d available=%d", __LINE__, f.available());
			result = false;
		}
	}

	if (result) {
		if (f.eof()) {
			Log.error("failure line %d eof=%d", __LINE__, f.eof());
			result = false;
		}
	}

	if (result) {

		s = f.readStringUntil('\n');

		if (strcmp(s.c_str(), "line1\r") != 0) {
			Log.error("failure line %d len=%d s=%s", __LINE__, s.length(), s.c_str());
			result = false;
		}
	}

	if (result) {
		if (f.available() != 0) {
			Log.error("failure line %d available=%d", __LINE__, f.available());
			result = false;
		}
	}

	if (result) {
		if (!f.eof()) {
			Log.error("failure line %d eof=%d", __LINE__, f.eof());
			result = false;
		}
	}

 	if (result) {
		res = f.remove();
		if (res != SPIFFS_OK) {
			Log.error("failure line %d res=%ld", __LINE__, res);
			result = false;
		}
	}

	if (result) {
		f = fs.openFile("t3.txt", SPIFFS_O_RDWR);
		if (f.isValid()) {
			// Should not exist now
			f.close();
			Log.error("failure line %d", __LINE__);
			result = false;
		}
	}

	if (result) {
		Log.info("testWiring passed");
	}

	return result;
}

bool testLarge() {
	bool result = true;


	SpiffsParticleFile f = fs.openFile("t4.txt", SPIFFS_O_RDWR|SPIFFS_O_CREAT);
	if (!f.isValid()) {
		Log.error("failure line %d", __LINE__);
		result = false;
	}


	uint8_t buf[256];
	srand(0);

	{
		LogTime time("write 64K in 256 byte pages");
		for(size_t ii = 0; ii < 256; ii++) {
			for(size_t jj = 0; jj < 256; jj++) {
				buf[jj] = (u8_t) rand();
			}
			f.write(buf, sizeof(buf));
		}
	}

	f.seekStart();
	srand(0);

	{
		LogTime time("read 64K in 256 byte pages");
		for(size_t ii = 0; ii < 256; ii++) {
			f.readBytes((char *)buf, sizeof(buf));
			for(size_t jj = 0; jj < 256; jj++) {
				if (buf[jj] != (u8_t) rand()) {
					Log.info("incorrect data ii=%u jj=%u value=%02x", ii, jj, buf[jj]);
					result = false;
					break;
				}
			}
		}
	}

	f.close();

	if (result) {
		Log.info("testLarge passed");
	}

	return result;
}

void runFullTestSuite() {
	Log.info("starting runFullTestSuite");


	Log.info("about to erase flash sectors");
	fs.erase();

	// Need to call mount to initialize things
	s32_t res = fs.mount(NULL);
	if (res != SPIFFS_ERR_NOT_A_FS) {
		Log.error("failure line %d res=%ld", __LINE__, res);
		return;
	}

	// Format the disk to start from scratch
	{
		LogTime time("format");
		res = fs.format();
	}
	if (res != SPIFFS_OK) {
		Log.error("failure line %d res=%ld", __LINE__, res);
		return;
	}

	{
		LogTime time("mount");
		res = fs.mount(NULL);
	}
	if (res != SPIFFS_OK) {
		Log.error("failure line %d res=%ld", __LINE__, res);
		return;
	}

	bool bResult = testSimple();
	if (!bResult) {
		return;
	}

	bResult = testAppend();
	if (!bResult) {
		return;
	}

	bResult = testDir1();
	if (!bResult) {
		return;
	}

	bResult = testWiring();
	if (!bResult) {
		return;
	}

	bResult = testLarge();
	if (!bResult) {
		return;
	}

	// Check the file system
	{
		LogTime time("filesystem check");
		fs.check();
	}

	fs.unmount();
	Log.info("test completed");
}



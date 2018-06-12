#include "Particle.h"

#include "SpiffsParticleRK.h"

/*
 * This example can only be used on the Photon/P1 because it uses Wi-Fi TCP server mode
 *
 * It illustrates how to handle the file system in stop sleep (pin + time mode). In this mode,
 * all variables are preserved so you don't need to re-mount the file system, which makes it
 * very efficient to wake up, write to flash, and go back to sleep.
 *
 * However, for illustration purposes, after waking up, this test program waits for 60 seconds
 * before going back to sleep.
 *
 * During this time, you can connect to port 8000 using telnet or nc and the contents of the test
 * file will be output.
 *
 * $ nc 192.168.2.43 8000
 * counter=0 millis=10000 time=Tue Jun 12 12:08:41 2018
 * counter=1 millis=20000 time=Tue Jun 12 12:08:51 2018
 * counter=2 millis=30000 time=Tue Jun 12 12:09:01 2018
 * counter=0 millis=10000 time=Tue Jun 12 12:11:13 2018
 * counter=1 millis=20000 time=Tue Jun 12 12:11:23 2018
 * counter=2 millis=30000 time=Tue Jun 12 12:11:33 2018
 * counter=3 millis=40000 time=Tue Jun 12 12:11:43 2018
 * counter=4 millis=50000 time=Tue Jun 12 12:11:53 2018
 * counter=5 millis=60000 time=Tue Jun 12 12:12:03 2018
 * counter=6 millis=70000 time=Tue Jun 12 12:12:44 2018
 * counter=7 millis=80000 time=Tue Jun 12 12:12:54 2018
 * counter=8 millis=90000 time=Tue Jun 12 12:13:04 2018
 */

SYSTEM_THREAD(ENABLED);

// Pick a debug level from one of these two:
// SerialLogHandler logHandler;
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Chose a flash configuration:
SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
// SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins)
// SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module

// Create an object for the SPIFFS file system
SpiffsParticle fs(spiFlash);

const unsigned long SLEEP_PERIOD_SEC = 30;
const unsigned long WAKE_PERIOD_MS = 60000;
const unsigned long WRITE_PERIOD_MS = 10000;

unsigned long lastSleepMs = 0;
unsigned long lastWriteMs = 0;
int counter = 0;
bool wifiReady = false;

SpiffsParticleFile testFile;
TCPServer server = TCPServer(8000);

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
	if (WiFi.ready()) {
		if (!wifiReady) {
			// Was just turned on, restart listener. This must be done every time we reconnect to Wi-Fi
			// including waking up from sleep
			server.begin();
			wifiReady = true;
		}
		else {
			// When a connection to make to port 80, dump out the contents of the test file and then close the connection.
			TCPClient client = server.available();
			if (client.connected()) {
				if (testFile.isValid()) {
					unsigned char buf[256];

					testFile.seekStart();
					while(true) {
						size_t count = testFile.readBytes((char *)buf, sizeof(buf));
						if (count == 0) {
							break;
						}
						client.write(buf, count);
					}
				}
				client.stop();
			}
		}
	}
	else {
		wifiReady = false;
	}

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

		// Write any outstanding data on all open files to flash before going to sleep
		fs.flush();

		// Clear this flag so we restart the TCP server listener after reconnecting to Wi-Fi
		wifiReady = false;

		System.sleep(WKP, RISING, SLEEP_PERIOD_SEC);

	}
}



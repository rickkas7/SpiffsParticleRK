/**
 * SpiffsParticleRK - Particle wrapper for SPIFFS library
 *
 * Port and this wrapper: https://github.com/rickkas7/SpiffsParticleRK
 * Original SPIFFS library: https://github.com/pellepl/spiffs/
 *
 * License: MIT (both)
 */


#include "SpiffsParticleRK.h"

static Logger log("app.spiffs");

static os_mutex_t _spiffsMutex = []() {
	os_mutex_t m;
	os_mutex_create(&m);
	return m;
}();


SpiffsParticle::SpiffsParticle(SpiFlashBase &flash) : flash(flash) {

	//
	config.hal_read_f = readCallbackStatic;
	config.hal_write_f = writeCallbackStatic;
	config.hal_erase_f = eraseCallbackStatic;

	// These are the minimum values you can use
	config.phys_erase_block = flash.getSectorSize();
	config.log_page_size = flash.getPageSize();

	config.phys_size = 1024 * 1024; // Just a guess, override for other chips
	config.phys_addr = 0;
	config.log_block_size = flash.getSectorSize();

	fs.user_data = this;

}

SpiffsParticle::~SpiffsParticle() {

}

s32_t SpiffsParticle::mount(spiffs_check_callback callback) {
	if (workBuffer == 0) {
		workBuffer = static_cast<u8_t *>(malloc(2 * config.log_page_size));
		if (workBuffer == 0) {
			return SPIFFS_ERR_OUT_OF_MEMORY;
		}
	}
	if (fdBuffer == 0) {
		fdBufferSize = 32 * maxOpenFiles;
		fdBuffer = static_cast<u8_t *>(malloc(fdBufferSize));
		if (fdBuffer == 0) {
			return SPIFFS_ERR_OUT_OF_MEMORY;
		}
	}
	if (cacheBuffer == 0) {
		cacheBufferSize = (config.log_page_size + 32) * cachePages + 40;
		cacheBuffer = static_cast<u8_t *>(malloc(cacheBufferSize));
		if (cacheBuffer == 0) {
			return SPIFFS_ERR_OUT_OF_MEMORY;
		}
	}
	userCheckCallback = callback; // may be null

	return SPIFFS_mount(&fs, &config, workBuffer, fdBuffer, fdBufferSize, cacheBuffer, cacheBufferSize, checkCallbackStatic);
}

void SpiffsParticle::unmount() {
	SPIFFS_unmount(&fs);

	free(workBuffer);
	workBuffer = 0;

	free(fdBuffer);
	fdBuffer = 0;

	free(cacheBuffer);
	cacheBuffer = 0;
}

s32_t SpiffsParticle::mountAndFormatIfNecessary(spiffs_check_callback callback) {
	s32_t res = mount(NULL);
	log.info("mount res=%ld", res);

	if (res == SPIFFS_ERR_NOT_A_FS) {
		res = format();
		log.info("format res=%ld", res);

		if (res == SPIFFS_OK) {
			res = mount(NULL);
			log.info("mount after format res=%ld", res);
		}
	}
	return res;
}


s32_t SpiffsParticle::erase() {
	if (mounted()) {
		return SPIFFS_ERR_MOUNTED;
	}

	eraseCallback(config.phys_addr, config.phys_size);

	return SPIFFS_OK;
}


s32_t SpiffsParticle::readCallback(u32_t addr, u32_t size, u8_t *dst) {
	flash.readData(addr, dst, size);

	if (lowLevelDebug) {
		if (size == 2) {
			log.trace("read addr=0x%lx size=%lu data=%02x%02x", addr, size, dst[0], dst[1]);
		}
		else {
			log.trace("read addr=0x%lx size=%lu", addr, size);
		}
	}

	return SPIFFS_OK;
}

s32_t SpiffsParticle::writeCallback(u32_t addr, u32_t size, u8_t *src) {
	flash.writeData(addr, src, size);

	if (lowLevelDebug) {
		if (size == 2) {
			log.trace("write addr=0x%lx size=%lu data=%02x%02x", addr, size, src[0], src[1]);
		}
		else {
			log.trace("write addr=0x%lx size=%lu", addr, size);
		}
	}

	return SPIFFS_OK;
}

s32_t SpiffsParticle::eraseCallback(u32_t addr, u32_t size) {

	size_t sectorSize = flash.getSectorSize();

	while(size >= sectorSize) {
		if (lowLevelDebug) {
			log.trace("erase sector addr=0x%lx size=%lu", addr, sectorSize);
		}
		flash.sectorErase(addr);
		addr += sectorSize;
		size -= sectorSize;
	}

	return SPIFFS_OK;
}

void SpiffsParticle::checkCallback(spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2) {

	if (userCheckCallback) {
		userCheckCallback(&fs, type, report, arg1, arg2);
	}

	// This operation can take a while, so make sure in non-threaded mode the cloud doesn't
	// disconnect by calling Particle.process
	Particle.process();
}



// [static]
s32_t SpiffsParticle::readCallbackStatic(struct spiffs_t *fs, u32_t addr, u32_t size, u8_t *dst) {
	SpiffsParticle *This = static_cast<SpiffsParticle *>(fs->user_data);
	return This->readCallback(addr, size, dst);
}

// [static]
s32_t SpiffsParticle::writeCallbackStatic(struct spiffs_t *fs, u32_t addr, u32_t size, u8_t *src) {
	SpiffsParticle *This = static_cast<SpiffsParticle *>(fs->user_data);
	return This->writeCallback(addr, size, src);
}

// [static]
s32_t SpiffsParticle::eraseCallbackStatic(struct spiffs_t *fs, u32_t addr, u32_t size) {
	SpiffsParticle *This = static_cast<SpiffsParticle *>(fs->user_data);
	return This->eraseCallback(addr, size);
}

// [static]
void SpiffsParticle::checkCallbackStatic(struct spiffs_t *fs, spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2) {
	SpiffsParticle *This = static_cast<SpiffsParticle *>(fs->user_data);
	This->checkCallback(type, report, arg1, arg2);
}


extern "C"
void spiffsParticleInfoLog(const char *fmt, ...) {
	if (log.isInfoEnabled()) {
		va_list args;
		va_start(args, fmt);
		log_printf_v(LOG_LEVEL_INFO, "app.spiffs", nullptr, fmt, args);
		va_end(args);
	}
}

extern "C"
void spiffsParticleTraceLog(const char *fmt, ...) {
	if (log.isTraceEnabled()) {
		va_list args;
		va_start(args, fmt);
		log_printf_v(LOG_LEVEL_TRACE, "app.spiffs", nullptr, fmt, args);
		va_end(args);
	}
}

extern "C"
void spiffsParticleLock() {
	os_mutex_lock(_spiffsMutex);
}

extern "C"
void spiffsParticleUnlock() {
	os_mutex_unlock(_spiffsMutex);
}


SpiffsParticleFile::SpiffsParticleFile() {

}

SpiffsParticleFile::SpiffsParticleFile(spiffs_t *fs, spiffs_file fh) : fs(fs), fh(fh) {

}

SpiffsParticleFile::~SpiffsParticleFile() {

}

SpiffsParticleFile::SpiffsParticleFile(const SpiffsParticleFile &other) {
	fs = other.fs;
	fh = other.fh;
}
SpiffsParticleFile &SpiffsParticleFile::operator=(const SpiffsParticleFile &other) {
	fs = other.fs;
	fh = other.fh;
	return *this;
}

int SpiffsParticleFile::available() {
	spiffs_stat stat;

	SPIFFS_fstat(fs, fh, &stat);

	return stat.size - SPIFFS_tell(fs, fh);
}


int SpiffsParticleFile::read() {
	char buf[1];

	SPIFFS_read(fs, fh, buf, sizeof(buf));

	return buf[0];
}
int SpiffsParticleFile::peek() {
	char buf[1];

	SPIFFS_read(fs, fh, buf, sizeof(buf));
	SPIFFS_lseek(fs, fh, -1, SPIFFS_SEEK_CUR);

	return buf[0];

}
void SpiffsParticleFile::flush() {
	SPIFFS_fflush(fs, fh);
}

size_t SpiffsParticleFile::readBytes( char *buffer, size_t length) {
	s32_t res = SPIFFS_read(fs, fh, buffer, length);
	if (res < 0) {
		return 0;
	}
	return (size_t) res;
}

size_t SpiffsParticleFile::write(uint8_t val) {
	uint8_t buf[1];
	buf[0] = val;

	return write(buf, 1);
}

size_t SpiffsParticleFile::write(const uint8_t *buffer, size_t size) {
	s32_t res = SPIFFS_write(fs, fh, const_cast<void *>(reinterpret_cast<const void *>(buffer)), size);
	if (res < 0) {
		return 0;
	}
	return (size_t) res;
}


s32_t SpiffsParticleFile::length() {
	spiffs_stat stat;
	s32_t res = SPIFFS_fstat(fs, fh, &stat);
	if (res == SPIFFS_OK) {
		res = (s32_t) stat.size;
	}
	return res;
}


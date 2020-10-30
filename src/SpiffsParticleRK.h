/**
 * SpiffsParticleRK - Particle wrapper for SPIFFS library
 *
 * Port and this wrapper: https://github.com/rickkas7/SpiffsParticleRK
 * Original SPIFFS library: https://github.com/pellepl/spiffs/
 *
 * License: MIT (both)
 */

#ifndef __SPIFFSPARTICLERK_H
#define __SPIFFSPARTICLERK_H

#include "Particle.h"

#include "spiffs.h"

// Requires the SpiFlashRK library to access SPI flash chips. This library can also be used to access the
// P1 external flash chip in the P1 module.
// https://github.com/rickkas7/SpiFlashRK
#include "SpiFlashRK.h"


/**
 * @brief Extension of Arduino/Wiring Stream/Print manipulating a single SPIFFS file
 *
 * You can use either this API, or the native SPIFFS API, which looks basically like the Unix file API.
 */
class SpiffsParticleFile : public Stream {
public:
	/**
	 * @brief You normally don't need to instantiate one of these, you use the `openFile` method of SpiffsParticle instead.
	 */
	SpiffsParticleFile();

	/**
	 * @brief You normally don't need to instantiate one of these, you use the `openFile` method of SpiffsParticle instead.
	 */
	SpiffsParticleFile(spiffs_t *fs, spiffs_file fh);

	/**
	 * @brief Destructor. Note that this only destroys the container, the underlying file handle is still open and valid.
	 */
	virtual ~SpiffsParticleFile();

	/**
	 * @brief You normally don't need to instantiate one of these, you use the `openFile` method of SpiffsParticle instead.
	 */
	SpiffsParticleFile(const SpiffsParticleFile &other);

	/**
	 * @brief You can copy this object, so it's safe to assign it to a local variable.
	 *
	 * This object is only a container for the file handle; it doesn't do any reference counting
	 * and the file handle is only closed if you call the close() method; deleting this object
	 * or letting it go out of scope doesn't close the file handle.
	 */
	SpiffsParticleFile &operator=(const SpiffsParticleFile &other);

	/**
	 * @brief Returns the number of bytes available to read from the current file position
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual int available();

	/**
	 * @brief Read a single byte from the current file position and increment the file position.
	 *
	 * @return a byte value 0 - 255 or -1 if at the end of file.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual int read();

	/**
	 * @brief Read a single byte from the current file position, but do not increment the file position.
	 *
	 * @return a byte value 0 - 255 or -1 if at the end of file.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual int peek();

	/**
	 * @brief Write any buffered output.
	 *
	 * It is not necessary to flush before close, as close will automatically flush any output if necessary.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual void flush();

	/**
	 * @brief Read a specified number of bytes from the file.
	 *
	 * @param buffer The buffer to write to
	 *
	 * @param length The number of bytes to read. It may read less than that.
	 *
	 * @return The number of bytes read, which will be 0 to length bytes.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual size_t readBytes( char *buffer, size_t length);

	/**
	 * @brief Write a single byte to the current file position and increment the file position.
	 *
	 * @param c The byte to write. Can write binary or text data.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual size_t write(uint8_t c);

	/**
	 * @brief Write bytes to the current file position and increment the file position.
	 *
	 * @param buffer The buffer to write. Can write binary or text data.
	 *
	 * @param length The number of bytes to write.
	 *
	 * @return The number of bytes written, typically this will be length.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual size_t write(const uint8_t *buffer, size_t size);

    /**
     * @brief Adjust the file position for reading or writing
     *
     * @param offs Offset, can be positive or negative
     *
     * @param whence Offset relative to what:
     *
     * - SPIFFS_SEEK_SET the beginning of the file
	 * - SPIFFS_SEEK_CUR the current file position
	 * - SPIFFS_SEEK_END the end of the file
	 *
	 * @return The resulting file offset or a negative SPIFFS_ERR code.
	 *
     * This is a wrapper for the SPIFFS call, which emulates the Unix/POSIX API.
     */
	inline s32_t lseek(s32_t offs, int whence) { return SPIFFS_lseek(fs, fh, offs, whence); };

	/**
	 * @brief Returns true of the file position is currently at the end of the file
	 *
	 * @return true if at end of file, false if not
	 */
	inline bool eof() { return SPIFFS_eof(fs, fh) != 0; };

	/**
	 * @brief Return the current file position from the start of the file
	 *
	 * @return file position (0 = at the start of the file)  or a negative error code
	 *
     * This is a wrapper for the SPIFFS call, which emulates the Unix/POSIX API.
	 */
	inline s32_t tell() { return SPIFFS_tell(fs, fh); };

	/**
	 * @brief Returns the length of the file in bytes
	 *
	 * @return the length of the file in bytes or a negative error code
	 */
	s32_t length();

	/**
	 * @brief Delete the currently open file.
	 *
	 * Note that the SpiffsParticle class has a method to delete a file by filename, this method is
	 * provided if you already have the file open.
	 *
	 * @return SPIFFS_OK (0) on success, or a negative SPIFFS_ERR code.
	 */
	inline s32_t remove() { return SPIFFS_fremove(fs, fh); };

	/**
	 * @brief Truncate the currently open file to the specified number of bytes.
	 *
	 * Note that in POSIX, you can truncate a file to make it longer than the current file length but in
	 * SPIFFS you can only truncate a file to be equal to or less than the current file length.
	 *
	 * @return SPIFFS_OK (0) on success, or a negative SPIFFS_ERR code.
	 */
	inline s32_t truncate(s32_t len) { return SPIFFS_ftruncate(fs, fh, len); };

	/**
	 * @brief Convenience function to move the file position to the beginning of the file
	 */
	inline void seekStart() { lseek(0, SPIFFS_SEEK_SET); };

	/**
	 * @brief Convenience function to move the file position to the end of the file
	 */
	inline void seekEnd() { lseek(0, SPIFFS_SEEK_END); };

	/**
	 * @brief Close this file. Make sure you close a file when done as there are a finite number of
	 * open file available.
	 */
    inline void close() { SPIFFS_close(fs, fh); };

	/**
	 * @brief Returns true if this object appears to be valid
	 *
	 * This is mainly used when using the openFile method. Since this object is returned, you use the
	 * isValid() method to find out if the openFile succeeded. It might not if you try to open without
	 * create a file that does not exist, for example.
	 */
    inline bool isValid() const { return fs != 0 && fh >= 0; };

    /**
     * @brief Get the underlying SPIFFS file handle for this file
     */
    inline operator spiffs_file() { return fh; };

    // Arduino Stream methods

    /*!
     * \fn String readString();
     *
     * \brief Read the entire file a String object
     *
     * You should not do this for large files and the entire file needs to be read into RAM.
     */


private:
    spiffs_t *fs = 0;
    spiffs_file fh = 0;
};

/**
 * @brief C++ wrapper for the SPIFFS library for the Particle platform
 */
class SpiffsParticle {
public:
	/**
	 * @brief Create a SpiffsParticle file system object. Note that you must mount the file system before using it!
	 *
	 * It's safe (and common) to create the SpiffsParticle object as a global variable.
	 */
	SpiffsParticle(SpiFlashBase &flash);
	virtual ~SpiffsParticle();

	/**
	 * @brief Sets the size of the flash file system in bytes, relative to the physical start address.
	 */
	inline SpiffsParticle &withPhysicalSize(size_t value) { config.phys_size = value; return *this; };

	/**
	 * @brief Sets the start address in the flash for the file system (default: 0)
	 */
	inline SpiffsParticle &withPhysicalAddr(size_t value) { config.phys_addr = value; return *this; };

	/**
	 * @brief Sets the physical block size (default: 4096)
	 *
	 * This is the size of the erase block, or the sector size in the SpiFlash class. It can't be smaller than
	 * the sector size, and it can't be larger than the logicalBlockSize, so pretty much 4096 is what you should
	 * leave it at.
	 */
	inline SpiffsParticle &withPhysicalBlockSize(size_t value) { config.phys_erase_block = value; return *this; };

	/**
	 * @brief Sets the logical block size (default: 4096)
	 *
	 * This can't be smaller than the physical block size or sector size (4096), but it could be larger. Making it
	 * larger might make sense if you have a small number of really large files, but usually 4096 is a reasonable
	 * value.
	 * 
	 * The logical block number is a uint16_t (0-65535) so at 4096 bytes this supports a file system up to 
	 * 268,435,456 bytes (256 Mbyte). Large file systems will require a larger logical block size.
	 */
	inline SpiffsParticle &withLogicalBlockSize(size_t value) { config.log_block_size = value; return *this; };

	/**
	 * @brief Sets the logical page size (default: 256)
	 *
	 * This must be greater than or equal to the physical page size, which is typically 256 for most flash chips.
	 * Note that there's a work buffer of 2 * logical page size required for mounting a volume. It's rarely helpful
	 * to change this from the default except for large flash chips. It can't be larger than the logical block size.
	 * 
	 * The logical page number is a uint16_t (0-65535) so at 256 bytes, this supports a file system up to 
	 * 16,777,216 bytes (16 Mbyte). 
	 * 
	 * For a large flash chip (256 Mbit/32 Mbyte, like the MX25L25645G) you can set the logical page size to 512
	 * to allow the use of the whole chip.
	 */
	inline SpiffsParticle &withLogicalPageSize(size_t value) { config.log_page_size = value; return *this; };

	/**
	 * @brief Sets the maximum number of open files (default: 4)
	 *
	 * Each open file descriptor requires 32 bytes of RAM. For the default of 4, this is 128 bytes allocated at mount time.
	 *
	 * Note that you must call this before mount(). Calling it after will have no effect.
	 */
	inline SpiffsParticle &withMaxOpenFiles(size_t value) { maxOpenFiles = value; return *this; };

	/**
	 * @brief Sets the desired not of cache pages (default: 2)
	 *
	 * The cache requires (logical page size + 32) * cachePages + 40 bytes. For the default logical page size
	 * of 256 and cache pages of 2, this is 616 bytes allocated at mount time.
	 *
	 * Note that you must call this before mount(). Calling it after will have no effect.
	 */
	inline SpiffsParticle &withCachePages(size_t value) { cachePages = value; return *this; };

	/**
	 * @brief Enable (or disable) low level debug mode
	 *
	 * This mainly logs individual read and write calls. The SPIFFS file system likes to write two byte values
	 * as part of the magic, and low-level debug will print these out. If you're having weird file system
	 * corruption errors, using this along with enabling trace logging may be helpful.
	 */
	inline SpiffsParticle &withLowLevelDebug(bool value = true) { lowLevelDebug = value; return *this; };

	/**
	 * @brief Mount the file system
	 *
	 * @return SPIFFS_OK (0) on success or a negative error code. The most common error is SPIFFS_ERR_NOT_A_FS (-10025)
	 * which means you need to format the file system.
	 *
	 * Note that you must always try to mount the file system before performing any other operations, including format.
	 * It allocates buffers for the work buffer, file handles, and cache buffer, so you should do it early in setup
	 * if possible to make sure there is memory available.
	 *
	 * In addition to the standard SPIFFS_ERR errors, the SpiffsParticle port can also return SPIFFS_ERR_OUT_OF_MEMORY
	 * if the buffers cannot be allocated during mount.
	 *
	 * The memory allocated to buffers when mounting is:
	 *
	 * - Work buffers (2 * logical page size), default is 2 * 256 = 512
	 * - File descriptor buffers (32 * max open files), default is 32 * 4 = 128
	 * - Cache (logical page size + 32) * cachePages + 40 byte, default is (256 + 32) * 2 + 40 = 616
	 *
	 * Thus the total RAM allocated during mount is by default is 1256 bytes.
	 */
	s32_t mount(spiffs_check_callback callback = 0);

	/**
	 * @brief Unmount the file system. All file handles will be flushed of any cached writes and closed.
	 *
	 * This also frees the file descriptor, work, and cache buffers.
	 */
	void unmount();

	/**
	 * @brief Mount the file system and format if necessary
	 */
	s32_t mountAndFormatIfNecessary(spiffs_check_callback callback = 0);

	/**
	 * @brief Format the file system
	 *
	 * You typically do this when mount returns SPIFFS_ERR_NOT_A_FS. Note that you cannot format a valid file system,
	 * you'll need to erase the sectors first. Also, you must try mounting again after format.
	 */
	inline s32_t format() { return SPIFFS_format(&fs); };

	/**
	 * @brief Erase the sectors used by the file system. Obviously all data will be lost.
	 *
	 * This function returns error SPIFFS_ERR_MOUNTED if the file system is currently mounted; you must
	 * unmount it before erasing.
	 */
	s32_t erase();

	/**
	 * @brief Flush all cached writes for all file handles, but leave the files open and the volume mounted.
	 *
	 * You'd typically do this before doing a stop mode sleep (pin + time) where the RAM contents are preserved
	 * but you want to make sure the data is saved. This saves having to mount the volume again.
	 *
	 * If you are using SLEEP_MODE_DEEP you should use unmount() instead.
	 */
	inline void flush() { SPIFFS_flush(&fs); };

	/**
	 * @brief Creates a new file.
	 *
	 * @param path          the path of the new file
	 * @param mode          ignored, for posix compliance. This is an optional parameter.
	 *
	 * Note that there are no subdirectories in SPIFFS and the maximum filename length is 32.
	 */
	inline s32_t creat(const char *path, spiffs_mode mode = 0777) { return SPIFFS_creat(&fs, path, mode); };

	/**
	 * @brief Opens or creates a file.
	 *
	 * @param path          the path of the new file
	 * @param flags         the flags for the open command, can be combinations of
	 *                      SPIFFS_O_APPEND, SPIFFS_O_TRUNC, SPIFFS_O_CREAT, SPIFFS_O_RDONLY,
	 *                      SPIFFS_O_WRONLY, SPIFFS_O_RDWR, SPIFFS_O_DIRECT, SPIFFS_O_EXCL
	 * @param mode          ignored, for posix compliance. This is an optional parameter.
	 *
	 * You can combine these options, so for example to create (if necessary) and open, use SPIFFS_O_CREAT|SPIFFS_O_RDWR.
	 *
	 * Note that there are no subdirectories in SPIFFS and the maximum filename length is 32.
	 */
	inline spiffs_file open(const char *path, spiffs_flags flags, spiffs_mode mode = 0777) { return SPIFFS_open(&fs, path, flags, mode); };

	/**
	 * @brief Open a file and return a SpiffsParticleFile object to easily manipulate it using Arduino/Wiring style calls.
	 *
	 * @param path          the path of the new file
	 * @param flags         the flags for the open command, can be combinations of
	 *                      SPIFFS_O_APPEND, SPIFFS_O_TRUNC, SPIFFS_O_CREAT, SPIFFS_O_RDONLY,
	 *                      SPIFFS_O_WRONLY, SPIFFS_O_RDWR, SPIFFS_O_DIRECT, SPIFFS_O_EXCL
	 * @param mode          ignored, for posix compliance. This is an optional parameter.
	 *
	 * You can combine these options, so for example to create (if necessary) and open, use SPIFFS_O_CREAT|SPIFFS_O_RDWR.
	 *
	 * Note that there are no subdirectories in SPIFFS and the maximum filename length is 32.
	 */
	inline SpiffsParticleFile openFile(const char *path, spiffs_flags flags, spiffs_mode mode = 0777) { return SpiffsParticleFile(&fs, open(path, flags, mode)); };

	/**
	 * @brief Reads from given filehandle.
	 * @param fh            the filehandle
	 * @param buf           where to put read data
	 * @param len           how much to read
	 * @returns number of bytes read, or -1 if error
	 */
	inline s32_t read(spiffs_file fh, void *buf, s32_t len) { return SPIFFS_read(&fs, fh, buf, len); };

	/**
	 * @brief Writes to given filehandle.
	 * @param fh            the filehandle
	 * @param buf           the data to write
	 * @param len           how much to write
	 * @returns number of bytes written, or -1 if error
	 */
	inline s32_t write(spiffs_file fh, const void *buf, s32_t len) { return SPIFFS_write(&fs, fh, const_cast<void *>(buf), len); };

	/**
	 * @brief Moves the read/write file offset.
	 * @param fh            the filehandle
	 * @param offs          how much/where to move the offset. Can be negative.
	 * @param whence        if SPIFFS_SEEK_SET, the file offset shall be set to offset bytes
	 *                      if SPIFFS_SEEK_CUR, the file offset shall be set to its current location plus offset
	 *                      if SPIFFS_SEEK_END, the file offset shall be set to the size of the file plus offset, which should be negative
	 *
	 * @return Resulting offset is returned or negative if error.
	 */
	inline s32_t lseek(spiffs_file fh, s32_t offs, int whence) { return SPIFFS_lseek(&fs, fh, offs, whence); };

	/**
	 * @brief Check if EOF reached.
	 * @param fh            the filehandle of the file to check
	 *
	 */
	inline s32_t eof(spiffs_file fh) { return SPIFFS_eof(&fs, fh); };

	/**
	 * @brief Get position in file.
	 * @param fh            the filehandle of the file to check
	 */
	inline s32_t tell(spiffs_file fh) { return SPIFFS_tell(&fs, fh); };

	/**
	 * @brief Removes a file by path
	 * @param path          the path of the file to remove
	 */
	inline s32_t remove(const char *path) { return SPIFFS_remove(&fs, path); };

	/**
	 * @brief Removes a file by filehandle
	 * @param fh            the filehandle of the file to remove
	 */
	inline s32_t fremove(spiffs_file fh) { return SPIFFS_fremove(&fs, fh); };

	/**
	 * @brief Truncate a file by path
	 * @param path          the path of the file to remove
	 * @param len           the length to truncate to
	 *
	 * Note: In POSIX, len can be larger than the file size to make the file larger, but this
	 * does not work in SPIFFS. len must be less than or equal to the file size.
	 */
	inline s32_t truncate(const char *path, s32_t len) { return SPIFFS_truncate(&fs, path, len); };

	/**
	 * @brief Truncate a file by filehandle
	 * @param fh            the filehandle of the file to remove
	 * @param len           the length to truncate to
	 *
	 * Note: In POSIX, len can be larger than the file size to make the file larger, but this
	 * does not work in SPIFFS. len must be less than or equal to the file size.
	 * See: https://github.com/pellepl/spiffs/issues/107
	 */
	inline s32_t ftruncate(spiffs_file fh, s32_t len) { return SPIFFS_ftruncate(&fs, fh, len); };

	/**
	 * @brief Gets file status by path
	 * @param path          the path of the file to stat
	 * @param s             the stat struct to populate
	 *
	 * You will typically only need the type and name fields of the stats structure.
	 * type is SPIFFS_TYPE_FILE or SPIFFS_TYPE_DIR
	 * name is the name (c-string)
	 */
	inline s32_t stat(const char *path, spiffs_stat *s) { return SPIFFS_stat(&fs, path, s); };

	/**
	 * @brief Gets file status by filehandle
	 * @param fh            the filehandle of the file to stat
	 * @param s             the stat struct to populate
	 *
	 * You will typically only need the type and name fields of the stats structure.
	 * type is SPIFFS_TYPE_FILE or SPIFFS_TYPE_DIR
	 * name is the name (c-string)
	 */
	inline s32_t fstat(spiffs_file fh, spiffs_stat *s) { return SPIFFS_fstat(&fs, fh, s); };

	/**
	 * @brief Flushes all pending write operations from cache for given file
	 * @param fh            the filehandle of the file to flush
	 *
	 * It is not necessary to flush before close; close will flush any data if necessary.
	 */
	inline s32_t fflush(spiffs_file fh) { return SPIFFS_fflush(&fs, fh); };

	/**
	 * @brief Closes a filehandle. If there are pending write operations, these are finalized before closing.
	 * @param fh            the filehandle of the file to close
	 */
	inline s32_t close( spiffs_file fh) { return SPIFFS_close(&fs, fh); };

	/**
	 * @brief Renames a file
	 * @param old           path of file to rename
	 * @param newPath       new path of file
	 */
	inline s32_t rename(const char *old, const char *newPath) { return SPIFFS_rename(&fs, old, newPath); };

	/**
	 * @brief Returns last error of last file operation.
	 */
	inline s32_t spiffs_errno() { return SPIFFS_errno(&fs); };

	/**
	 * @brief Clears last error.
	 * @param fs            the file system struct
	 */
	inline void spiffs_clearerr() { SPIFFS_clearerr(&fs); };

	/**
	 * @brief Opens a directory stream corresponding to the given name.
	 * The stream is positioned at the first entry in the directory.
	 * On hydrogen builds the name argument is ignored as hydrogen builds always correspond
	 * to a flat file structure - no directories.
	 * @param name          the name of the directory
	 * @param d             pointer the directory stream to be populated
	 */
	inline spiffs_DIR *opendir(const char *name, spiffs_DIR *d) { return SPIFFS_opendir(&fs, name, d); };

	/**
	 * @brief Reads a directory into given spifs_dirent struct.
	 * @param d             pointer to the directory stream
	 * @param e             the dirent struct to be populated
	 * @returns null if error or end of stream, else given dirent is returned
	 */
	inline struct spiffs_dirent *readdir(spiffs_DIR *d, struct spiffs_dirent *e) { return SPIFFS_readdir(d, e); };

	/**
	 * @brief Closes a directory stream
	 * @param d             the directory stream to close
	 */
	inline s32_t closedir(spiffs_DIR *d) { return SPIFFS_closedir(d); };

	/**
	 * @brief Runs a consistency check on given filesystem.
	 *
	 * This takes a while to run so you probably don't want to do this very often.
	 */
	inline s32_t check() { return SPIFFS_check(&fs); };

	/**
	 * @brief Returns number of total bytes available and number of used bytes.
	 * This is an estimation, and depends on if there a many files with little
	 * data or few files with much data.
	 * NB: If used number of bytes exceeds total bytes, a SPIFFS_check should
	 * run. This indicates a power loss in midst of things. In worst case
	 * (repeated powerlosses in mending or gc) you might have to delete some files.
	 *
	 * @param total         total number of bytes in filesystem
	 * @param used          used number of bytes in filesystem
	 */
	inline s32_t info(u32_t *total, u32_t *used) { return SPIFFS_info(&fs, total, used); };

	/**
	 * @brief Returns nonzero if spiffs is mounted, or zero if unmounted.
	 */
	inline bool mounted() { return SPIFFS_mounted(&fs) != 0; };


	/**
	 * @brief Used internally to generate an info level log for app.spiffs
	 */
	static void infoLog(const char *fmt, ...);

	/**
	 * @brief Used internally to generate an trace level log for app.spiffs
	 */
	static void traceLog(const char *fmt, ...);

private:
	/**
	 * @brief Use internally to read from flash
	 */
	s32_t readCallback(u32_t addr, u32_t size, u8_t *dst);

	/**
	 * @brief Use internally to write to flash
	 */
	s32_t writeCallback(u32_t addr, u32_t size, u8_t *src);

	/**
	 * @brief Use internally to erase a flash sector
	 */
	s32_t eraseCallback(u32_t addr, u32_t size);

	/**
	 * @brief Use internally to during filesystem check
	 */
	void checkCallback(spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2);

	/**
	 * @brief Use internally to read from flash.
	 *
	 * Passed to SPIFFS. Assumes this (pointer to SpiffsParticle object instance) is in the user_data
	 * of the spiffs_t object.
	 */
	static s32_t readCallbackStatic(struct spiffs_t *fs, u32_t addr, u32_t size, u8_t *dst);

	/**
	 * @brief Use internally to write to flash.
	 *
	 * Passed to SPIFFS. Assumes this (pointer to SpiffsParticle object instance) is in the user_data
	 * of the spiffs_t object.
	 */
	static s32_t writeCallbackStatic(struct spiffs_t *fs, u32_t addr, u32_t size, u8_t *src);

	/**
	 * @brief Use internally to erase a flash sector.
	 *
	 * Passed to SPIFFS. Assumes this (pointer to SpiffsParticle object instance) is in the user_data
	 * of the spiffs_t object.
	 */
	static s32_t eraseCallbackStatic(struct spiffs_t *fs, u32_t addr, u32_t size);

	/**
	 * @brief used internally as the file system check callback
	 *
	 * Passed to SPIFFS. Assumes this (pointer to SpiffsParticle object instance) is in the user_data
	 * of the spiffs_t object.
	 */
	static void checkCallbackStatic(struct spiffs_t *fs, spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2);

	SpiFlashBase &flash;
	spiffs_config config;
	spiffs_t fs;
	size_t maxOpenFiles = 4;
	size_t cachePages = 4;
	u8_t *workBuffer = 0;
	u8_t *fdBuffer = 0;
	size_t fdBufferSize = 0;
	u8_t *cacheBuffer = 0;
	size_t cacheBufferSize = 0;
	bool lowLevelDebug = false;
	spiffs_check_callback userCheckCallback = 0;
};


#endif /* __SPIFFSPARTICLERK_H */

/**
 * @file SharedLib.h
 * @brief shared library loader
 * @author Christopher Han <xiphux@gmail.com>
 * @author Robin Vobruba <hoijui.quaero@gmail.com>
 *
 * Base shared library loading class definitiion
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#ifndef SHAREDLIB_H
#define SHAREDLIB_H

#include <string>

/**
 * @brief shared library base
 *
 * This is the abstract shared library class used for
 * polymorphic loading.  Platform-specifics should
 * derive from this.
 */
class SharedLib
{
public:
	/**
	 * @brief instantiate
	 * @param fileName file name as a C string
	 * @return platform-specific shared library class
	 */
	static SharedLib* Instantiate(const char* fileName);

	/**
	 * @brief instantiate
	 * @param fileName file name as a C++ string
	 * @return platform-specific shared library class
	 */
	static SharedLib* Instantiate(const std::string& fileName);

	/**
	 * Explicitly unload this shared library
	 * @brief unload
	 */
	virtual void Unload() = 0;

	/**
	 * Returns true if loading of the native library failed.
	 * @brief loadFailed
	 */
	virtual bool LoadFailed() = 0;

	/**
	 * @brief GetLibExtension
	 * @return "dll", "dylib" or "so" depending on OS
	 */
	static const char* GetLibExtension();

	/**
	 * @brief Find Address
	 * @param symbol function name (symbol) to locate
	 *
	 * Abstract so it must be implemented specifically by all platforms.
	 */
	virtual void* FindAddress(const char* symbol) = 0;

	virtual ~SharedLib();

protected:
	static void reportError(const char* errorMsg, const char* fileName = "???", int lineNumber = -1, const char* function = "???");
};

#endif // SHAREDLIB_H

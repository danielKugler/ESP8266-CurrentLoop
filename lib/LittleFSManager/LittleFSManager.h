#ifndef LITTLEFS_MANAGER_H
#define LITTLEFS_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>

class LittleFSManager {
   public:
    LittleFSManager(bool debug = false);
	bool debug;

	void println(const String& message);
    void begin();
    bool save(const String& key, int value);
    bool save(const String& key, const String& value);
    int read(const String& key, int defaultValue);
    String read(const String& key, const String& defaultValue);
    void clear(const String& key);
    void format();
};

#endif
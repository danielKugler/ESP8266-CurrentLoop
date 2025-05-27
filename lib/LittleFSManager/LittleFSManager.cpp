#include "LittleFSManager.h"

LittleFSManager::LittleFSManager(bool debug) : debug(debug) {}

void LittleFSManager::begin() {
    if (!LittleFS.begin()) {
        println("Fehler: LittleFS konnte nicht initialisiert werden.");
    } else {
        println("LittleFS erfolgreich initialisiert.");
    }
}

void LittleFSManager::println(const String& message) {
	if (debug) {
		Serial.println(message);
	}
}

/**
 * @brief Save an integer value associated with a key to LittleFS storage.
 *
 * This function opens a file corresponding to the given key in the LittleFS
 * storage. If the file is opened successfully, it writes the given value to
 * the file and closes the file. If the file cannot be opened, it returns false.
 *
 * @param key The key to be associated with the value.
 * @param value The integer value to be saved.
 *
 * @return true if the value is saved successfully, false otherwise.
 */
bool LittleFSManager::save(const String& key, int value) {
    File file = LittleFS.open("/" + key, "w");
    if (!file) {
        println("Fehler: Konnte Datei nicht öffnen.");
        return false;
    }

    file.print(value);
    file.close();
    println("Wert erfolgreich gespeichert: " + key + " -> " + String(value));
    return true;
}

/**
 * @brief Save a string value associated with a key to LittleFS storage.
 *
 * This function opens a file corresponding to the given key in the LittleFS
 * storage. If the file is successfully opened, it writes the provided string
 * value to the file and returns true. If the file cannot be opened, it logs
 * an error message and returns false.
 *
 * @param key The key under which the value will be stored.
 * @param value The string value to be saved.
 *
 * @return True if the value is successfully saved, false otherwise.
 */

bool LittleFSManager::save(const String& key, const String& value) {
    File file = LittleFS.open("/" + key, "w");
    if (!file) {
        println("Fehler: Konnte Datei nicht öffnen.");
        return false;
    }

    file.print(value);
    file.close();
    println("String erfolgreich gespeichert: " + key + " -> " + value);
    return true;
}

/**
 * Reads an integer value from LittleFS and returns it.
 *
 * If the key is not found, the defaultValue is returned.
 *
 * @param key The key under which the value is stored.
 * @param defaultValue The value that is returned if the key is not found.
 *
 * @return The value read as an integer.
 */
int LittleFSManager::read(const String& key, int defaultValue) {
    File file = LittleFS.open("/" + key, "r");
    if (!file) {
        println("Warnung: Schlüssel nicht gefunden: " + key);
        return defaultValue;
    }

    String value = file.readString();
    file.close();
    return value.toInt();
}

/**
 * @brief Retrieve a string value associated with a key from LittleFS storage.
 *
 * This function attempts to open a file corresponding to the given key in the 
 * LittleFS storage. If the file is found, it reads and returns the string content.
 * If the file is not found, it returns the provided default value.
 *
 * @param key The key corresponding to the value to be retrieved.
 * @param defaultValue The default string to return if the key is not found.
 *
 * @return The string value associated with the key, or the default value if the key
 *         is not found.
 */

String LittleFSManager::read(const String& key, const String& defaultValue) {
    File file = LittleFS.open("/" + key, "r");
    if (!file) {
        println("Warnung: Schlüssel nicht gefunden: " + key);
        return defaultValue;
    }

    String value = file.readString();
    file.close();
    return value;
}

/**
 * @brief Remove a specific key and its associated value from the LittleFS storage.
 *
 * This function checks if the specified key exists in the LittleFS storage. If it
 * does, the key and its value are removed. If the key is not found, a warning is
 * printed.
 *
 * @param key The key to be removed from the storage.
 *
 * @return void
 */

void LittleFSManager::clear(const String& key) {
    if (LittleFS.exists("/" + key)) {
        LittleFS.remove("/" + key);
        println("Schlüssel und zugehöriger Wert erfolgreich gelöscht: " + key);
    } else {
        println("Warnung: Schlüssel nicht gefunden: " + key);
    }
}

/**
 * @brief Format the LittleFS file system.
 *
 * This function formats the LittleFS storage, thereby deleting all files and
 * resetting the storage to its factory settings.
 *
 * @return void
 */
void LittleFSManager::format() {
    if (LittleFS.format()) {
        println("LittleFS erfolgreich formatiert.");
    } else {
        println("Fehler: LittleFS konnte nicht formatiert werden.");
    }
}

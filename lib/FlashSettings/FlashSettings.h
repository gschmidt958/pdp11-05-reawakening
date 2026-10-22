#ifndef FLASHSETTINGS_H
#define FLASHSETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

#define RW_MODE false
#define RO_MODE true

/*
 * FlashSettings
 *
 * Generic, reusable flash-backed configuration engine for
 * ESP32 NVS (Preferences). The application supplies a table
 * of Entry structs at construction time, each mapping a
 * front-panel address to a named key with default, min, and
 * max values.
 *
 * INITIALIZATION
 *   Init() writes default values for any keys not already
 *   present in NVS, making first-run behavior deterministic.
 *   SetDefaults() unconditionally overwrites all entries
 *   with their default values.
 *
 * ACCESS
 *   Get() and Set() operate by key name. Exists() checks
 *   whether a key is present in NVS. LookupByAddress() and
 *   LookupByName() search the table for an entry without
 *   touching NVS.
 *
 * SINGLETON
 *   A global instance pointer is provided for convenient
 *   access from other tasks without passing references.
 */

class FlashSettings
{
public:
  static FlashSettings* instance;

  struct Entry
  {
    uint16_t address;
    const char* name;
    int defaultValue;
    int minValue;
    int maxValue;
  };

  FlashSettings(const Entry* table, int tableSize, const char* ns = "settings")
    : table(table), tableSize(tableSize), namespaceName(ns)
  {
  }

  void Init();
  int Get(const char* name);
  void Set(const char* name, int value);
  void Remove(const char* name);
  bool Exists(const char* name);

  const Entry* LookupByAddress(uint16_t addr) const;
  const Entry* LookupByName(const char* name) const;

  void SetDefaults();

  const Entry* GetTable() const { return table; }
  int GetTableSize() const { return tableSize; }

private:
  const Entry* table;
  int tableSize;
  const char* namespaceName;
};

#endif // FLASHSETTINGS_H
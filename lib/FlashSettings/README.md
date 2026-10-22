# FlashSettings — Table-Driven Flash Configuration for ESP32

FlashSettings is a lightweight, deterministic configuration engine for ESP32
projects using the built-in Preferences (NVS) storage system. It provides a
table-driven, validated, and project-agnostic way to store persistent settings
with default values, min/max constraints, and lookup by name or address.

FlashSettings contains no UI logic and no task logic. It is designed to be
embedded inside any ESP32 application that needs structured configuration
storage. The Preferences API is managed entirely internally — callers never
open namespaces or call `prefs.begin()` directly.

---

## Why Use FlashSettings?

ESP32's Preferences API is simple but low-level:

- No validation
- No default management
- No structured table of known settings
- No lookup by address or name
- Namespace management left to the caller

FlashSettings adds these missing pieces while keeping the underlying storage
simple and deterministic.

---

## Features

- Table-driven configuration (address, name, default, min, max)
- Automatic first-run initialization — missing keys are written with defaults
- Read, write, delete, and existence-check individual settings by name
- Restore all settings to defaults with a single call
- Lookup entries by name or by front-panel address without touching NVS
- Full Preferences API encapsulation — callers never interact with it directly
- Each operation opens and closes its own local Preferences instance
- Optional singleton (`FlashSettings::instance`) for convenient global access
- Fully synchronous, no background tasks or interrupts

---

## API Reference

### Construction

```cpp
FlashSettings(const Entry* table, int tableSize, const char* ns = "settings");
```

Construct with an application-supplied settings table and an optional NVS
namespace name. The namespace defaults to `"settings"`.

### Initialization

```cpp
void Init();
```

Writes the default value for any key not already present in NVS. Call once
at startup. Keys that already exist are left unchanged, making repeated
calls safe.

```cpp
void SetDefaults();
```

Unconditionally overwrites all entries with their default values. Use to
implement a factory reset.

### Reading and Writing

```cpp
int  Get(const char* name);
void Set(const char* name, int value);
void Remove(const char* name);
bool Exists(const char* name);
```

All operations are by key name. `Get()` returns 0 if the key is not found.
`Exists()` checks NVS without reading the value. `Remove()` deletes the key
from NVS; the entry remains in the table.

### Table Lookup

```cpp
const Entry* LookupByAddress(uint16_t addr) const;
const Entry* LookupByName(const char* name) const;
```

Search the in-memory table without touching NVS. Returns `nullptr` if not
found. Use these to validate input or retrieve constraints (min, max) before
calling `Get()` or `Set()`.

```cpp
const Entry* GetTable() const;
int          GetTableSize() const;
```

Direct access to the table and its size, useful for iterating all entries.

### Singleton

```cpp
static FlashSettings* instance;
```

Optional global access point. Set this to your `FlashSettings` object after
construction to allow other modules to call `FlashSettings::instance->Get()`
without needing a reference passed to them.

---

## Defining the Settings Table

Define an array of `FlashSettings::Entry` structs. Each entry has five fields:

| Field          | Type           | Description                              |
|----------------|----------------|------------------------------------------|
| `address`      | `uint16_t`     | Front-panel switch address for this key  |
| `name`         | `const char*`  | NVS key name (max 15 characters)         |
| `defaultValue` | `int`          | Value written by `Init()` if key missing |
| `minValue`     | `int`          | Minimum valid value                      |
| `maxValue`     | `int`          | Maximum valid value                      |

Example:

```cpp
static FlashSettings::Entry flashTable[] =
{
    //  addr   name          default  min    max
    {   1,    "brightness",    8,      0,    15  },
    {   2,    "volume",        5,      0,    10  },
    {   3,    "dcOffset",     32,      0,    63  },
};

static const int flashTableSize =
    sizeof(flashTable) / sizeof(flashTable[0]);
```

NVS key names are limited to 15 characters by the ESP32 Preferences API.

---

## Usage Example

```cpp
#include "FlashSettings.h"
#include "FlashTable.h"

// Construct — typically in your setup task or main .ino
FlashSettings settings(flashTable, flashTableSize, "settings");

void setup()
{
    FlashSettings::instance = &settings;
    settings.Init(); // Write defaults for any missing keys
}

// Read a value anywhere in the application
int vol = FlashSettings::instance->Get("volume");

// Write a value
FlashSettings::instance->Set("volume", 12);

// Look up constraints before writing
const FlashSettings::Entry* entry = FlashSettings::instance->LookupByName("volume");
if (entry && newValue >= entry->minValue && newValue <= entry->maxValue)
    FlashSettings::instance->Set(entry->name, newValue);

// Restore all defaults
FlashSettings::instance->SetDefaults();
```

---

## NVS Namespace Management

Each method that accesses NVS opens a local `Preferences` instance, performs
its operation, and closes it immediately. The namespace is created
automatically by the ESP32 on first use. Callers never manage this directly.

---

## Notes and Constraints

- All values are stored as `int`. Other types are not currently supported.
- NVS key names must be 15 characters or fewer (ESP32 Preferences limit).
- `LookupByAddress()` and `LookupByName()` perform a linear search. For
  typical settings table sizes this is negligible.
- `Get()` returns 0 for keys not found in NVS. Call `Exists()` first if
  distinguishing "not set" from "set to zero" matters.
- Min/max constraints are stored in the table for validation by the caller.
  FlashSettings does not enforce them on `Set()` — enforcement is the
  caller's responsibility.
#include "FlashSettings.h"

FlashSettings* FlashSettings::instance = nullptr;

void FlashSettings::Init()
{
    Preferences prefs;
    prefs.begin(namespaceName, RW_MODE);

    for (int i = 0; i < tableSize; i++)
    {
        const Entry& e = table[i];

        if (!prefs.isKey(e.name))
            prefs.putInt(e.name, e.defaultValue);
    }

    prefs.end();
}

int FlashSettings::Get(const char* name)
{
    Preferences prefs;
    prefs.begin(namespaceName, RO_MODE);   // read-only
    int value = prefs.getInt(name, 0);
    prefs.end();
    return value;
}

void FlashSettings::Set(const char* name, int value)
{
    Preferences prefs;
    prefs.begin(namespaceName, RW_MODE);  // read/write
    prefs.putInt(name, value);
    prefs.end();
}

void FlashSettings::Remove(const char* name)
{
    Preferences prefs;
    prefs.begin(namespaceName, RW_MODE);  // read/write
    prefs.remove(name);
    prefs.end();
}

const FlashSettings::Entry* FlashSettings::LookupByAddress(uint16_t addr) const
{
    for (int i = 0; i < tableSize; i++)
    {
        if (table[i].address == addr)
            return &table[i];
    }
    return nullptr;
}

const FlashSettings::Entry* FlashSettings::LookupByName(const char* name) const
{
    for (int i = 0; i < tableSize; i++)
    {
        if (strcmp(table[i].name, name) == 0)
            return &table[i];
    }
    return nullptr;
}

void FlashSettings::SetDefaults()
{
    Preferences prefs;
    prefs.begin(namespaceName, RW_MODE);

    for (int i = 0; i < tableSize; i++)
    {
        const Entry& e = table[i];
        prefs.putInt(e.name, e.defaultValue);
    }

    prefs.end();
}

bool FlashSettings::Exists(const char* name)
{
    Preferences prefs;
    prefs.begin(namespaceName, RO_MODE);
    bool exists = prefs.isKey(name);
    prefs.end();
    return exists;
}
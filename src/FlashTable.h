#ifndef FLASHTABLE_H
#define FLASHTABLE_H

#include "FlashSettings.h"

// ============================================================================
// Flash Table — PDP‑11/05 Console
// ============================================================================

static FlashSettings::Entry flashTable[] =
{
    //  addr,   name,         default,  min,    max
    {   1,     "tick",          1,       0,      1 },    // 0 = silent, 1 = ticking
    {   2,     "chime",         1,       0,      3 },    // 0 = off, 1 = Westminster, 2 = bim-bam, 3 = hour strike
    {   3,     "cycleTime",    30,      10,   1800 },    // seconds between idle animations
    {   4,     "tempCal",       8,       0,     32 },    // onboard temperature calibration offset  16=0, 0=-16, 32=+16
    {   5,     "fixIntrvl",    15,       0,  65535 },    // fixed interval for timed events
    {   6,     "rndIntrvl",    15,       0,  65535 },    // random interval for timed events
    {   7,     "idlPattrn",     1,       0,      2 },    // idle animation pattern selector
    {   8,     "introTrack",    0,       0,     14 },    // 0 = random intro track
    {   9,     "introStyle",    1,       0,      3 },    // 0 = random, 1–3 = specific VU meter intro styles
    {  10,     "ovrBarCnt",    10,       0,     16 },    // Override countdown bar count
    {  11,     "fnlBarCnt",    10,       0,     16 },    // Final countdown bar count
    {  12,     "reboot",        1,       0,      1 },    // 1 = reboot following destruct sequence
    {  13,     "peakDecay",   150,       0,   1000 },    // peak hold decay rate
    {  14,     "BTVUGain",     30,       1,    100 },    // Bluetooth VU gain
    {  15,     "BTBinGain",     1,       1,      8 },    // Bluetooth Binary VU display gain
    {  16,     "pnlLckVol",    25,       1,     30 },    // Panel lock playback volume
    {  17,     "volume",       12,       1,     30 },    // BT201 playback volume
    {  18,     "stepCount",    20,       1,    300 },    // FF/Rew message count (each count = .2s, 300 = 1min)
    {  19,     "debounce",      8,       1,     32 },    // switch debounce time (ms)
    {  20,     "btVolReset",    1,       0,      1 },    // 1 = reset back to "volume" level when exiting BT
    {  21,     "dcOffset",   1978,       0,   4095 },    // ADC baseline offset for VU meter
    {  22,     "settleTime",    1,       1,      8 },    // switch settle time (μs)
};

static const int flashTableSize =
    sizeof(flashTable) / sizeof(flashTable[0]);

#endif

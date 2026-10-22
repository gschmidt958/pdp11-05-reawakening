#ifndef FLASHTASK_H
#define FLASHTASK_H

#include <TaskScheduler.h>
#include "BT201ParentTask.h"
#include "SwitchScanTask.h"
#include "FlashSettings.h"
#include "BT201Provisioning.h"

#define RW_MODE false
#define RO_MODE true

/*
 * FlashTask
 *
 * Provides a front-panel interface for viewing and editing
 * persistent settings stored in ESP32 flash (NVS). Derives
 * from BT201ParentTask to play confirmation tones on write
 * and reset operations.
 *
 * SETTINGS TABLE
 *   Each entry maps a front-panel switch address to a named
 *   NVS key with a default value and valid range. GetValue()
 *   and PutValue() provide direct access to individual
 *   settings by name for use by other tasks.
 *
 * OPERATION
 *   The front panel cycles through a sequence of states
 *   advanced by the CONTINUE sense switch:
 *     SelectAddress - displays and validates the switch
 *                     register against the settings table.
 *                     Run light indicates a valid address.
 *     SelectData    - displays the current stored value.
 *                     Switch register overrides the display.
 *                     Run light indicates a value in range.
 *     CommitData    - writes the selected value to NVS and
 *                     plays a confirmation tone.
 *     SelectReset   - displays 0xAAAA as a reset prompt.
 *     CommitReset   - restores all settings to defaults and
 *                     plays a confirmation tone.
 *     SelectVUCal   - displays the current DC offset value.
 *     Calibrate     - triggers VU meter calibration and
 *                     plays a confirmation tone on success.
 *
 * HIDDEN FUNCTION
 *   In SelectAddress, keying either the number and pressing CONTINUE
 *   triggers a hidden provisioning function that resets the BT201
 *   and plays a confirmation tone. The task remains in SelectAddress,
 *   so the user can trigger another function or proceed normally.
 *   A power cycle is required following the reset for the BT201
 *   settings to take effect.  On next power up, the system detects
 *   that the BT audio name has changed, and follows up with setting
 *   the appropriate chip modes.  Followng that, another power cycle
 *   is required.
 *
 *   0177570 (octal) — switch register address → RestoreFactoryDefaults()
 */

class FlashTask : public BT201ParentTask
{
public:
    FlashTask(const char* name, unsigned int period = 0);

    void Setup() override;
    void Init() override;
    void Cleanup() override;
    void Execute() override;

private:
    enum class FlashState
    {
        SelectAddress,
        SelectData,
        CommitData,
        SelectVUCal,
        Calibrate,
        SelectReset,
        CommitReset
    };

    FlashState flashState;
    bool entryAction = false;

    static uint16_t switches;
    static const FlashSettings::Entry* flashEntry;

    void SetState(FlashState state);
    void HandleSenseSwitch();
    void PlayConfirmationTone();
    static void OnToggle(void* instance);

    FlashSettings settings;
    BT201Provisioning provisioning;
};

#endif // FLASHTASK_H
#ifndef BLUETOOTHTASK_H
#define BLUETOOTHTASK_H

#include "SwitchScanTask.h"
#include "TaskSchedule.h"
#include "BT201ParentTask.h"
#include "VUMeterTask.h"

/*
 * BluetoothTask
 *
 * Activates and manages the Bluetooth audio mode for the
 * duration of the task's lifetime.
 *
 * LIFECYCLE
 *   On Init(), switches the BT201 module into Bluetooth
 *   mode with auto-switch enabled, and starts the VU meter.
 *   On Cleanup(), stops the VU meter, disables auto-switch,
 *   and returns the module to sound mode.
 *
 * VU METER STYLE
 *   The active VU meter display style is cycled by pressing
 *   the EXAMINE sense switch. Style selection persists across
 *   Init/Cleanup cycles via the saved style member.
 */

class BluetoothTask : public BT201ParentTask
{
  public:
    BluetoothTask(const char* name, unsigned int period = 0);

    void Init() override;
    void Cleanup() override;
    void Shutdown(Task* parentTask) override;

    void HandleSenseSwitch();

  private:
    VUMeterTask::Style style;
    uint8_t previousVUGain;
    uint8_t previousBinaryGain;

    void AdvanceVUMeterStyle();
    static void OnToggle(void* instance);
};

#endif // BLUETOOTHTASK_H
#include "BluetoothTask.h"
#include "SwitchScanTask.h"
#include "FlashSettings.h"
#include "BT201TransactionTask.h"
#include "VUMeterTask.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
BluetoothTask::BluetoothTask(const char* name, unsigned int period)
  : BT201ParentTask(name, period)
  , style(VUMeterTask::STYLE_L2RVU)
{
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------
void BluetoothTask::Init()
{
  SwitchScanTask::instance->Subscribe(BluetoothTask::OnToggle, this);

  BT201TransactionTask::instance->BluetoothMode();
  BT201TransactionTask::instance->AutoSwitchBluetooth(true);

  VUMeterTask::instance->SetStyle(style);

  previousVUGain = VUMeterTask::instance->GetVUGain();
  previousBinaryGain = VUMeterTask::instance->GetBinaryGain();

  VUMeterTask::instance->SetVUGain(FlashSettings::instance->Get("BTVUGain"));
  VUMeterTask::instance->SetBinaryGain(FlashSettings::instance->Get("BTBinGain"));

  taskScheduler->StartTask("VUMeter");
}

// ------------------------------------------------------------
// Shutdown (override)
// ------------------------------------------------------------
void BluetoothTask::Shutdown(Task* parentTask)
{
  // 1. Disable autoswitch BEFORE shutting down BT201 and Cleanup() runs.
  BT201TransactionTask::instance->AutoSwitchBluetooth(false);

  // 2. Disable BT audio
  //BT201TransactionTask::instance->DisableA2DP();
  //BT201TransactionTask::instance->SoftReset();
  
  // 3. Exit BT mode BEFORE shutting down BT201 and Cleanup() runs.
  BT201TransactionTask::instance->SoundMode();

  // 4. Conditionally assert prior volume since the AVRCP interaction with the phone changes the volume.
  if (FlashSettings::instance->Get("btVolReset"))
    BT201TransactionTask::instance->SetVolume(BT201TransactionTask::instance->GetVolume());

  // 5. Continue with the normal parent/child shutdown handshake
  BT201ParentTask::Shutdown(parentTask);
}

// ------------------------------------------------------------
// Cleanup
// ------------------------------------------------------------
void BluetoothTask::Cleanup()
{
  // Cleanup is now purely cosmetic — no BT mode transitions here.

  VUMeterTask::instance->SetVUGain(previousVUGain);
  VUMeterTask::instance->SetBinaryGain(previousBinaryGain);

  SwitchScanTask::instance->Unsubscribe(this);

  taskScheduler->EndTask("VUMeter");
}

// ------------------------------------------------------------
// Sense switch handling
// ------------------------------------------------------------
void BluetoothTask::HandleSenseSwitch()
{
  if (IsSenseSwitchActive(EXAMINE))
    AdvanceVUMeterStyle();
}

void BluetoothTask::AdvanceVUMeterStyle()
{
  int nextStyle = ((int)style + 1) % (int)VUMeterTask::STYLE_COUNT;
  style = (VUMeterTask::Style)nextStyle;
  VUMeterTask::instance->SetStyle(style);
}

// ------------------------------------------------------------
// Static callback
// ------------------------------------------------------------
void BluetoothTask::OnToggle(void* instance)
{
  static_cast<BluetoothTask*>(instance)->HandleSenseSwitch();
}

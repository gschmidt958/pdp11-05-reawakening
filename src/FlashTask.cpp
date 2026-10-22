#include "FlashTask.h"
#include "Registers.h"
#include "RunLight.h"
#include "VUMeterTask.h"
#include "BT201TransactionTask.h"
#include "FlashTable.h"

uint16_t FlashTask::switches = 0;
const FlashSettings::Entry* FlashTask::flashEntry = nullptr;

FlashTask::FlashTask(const char* name, unsigned int period)
  : BT201ParentTask(name, period),
    settings(flashTable, flashTableSize, "settings")
{
}

void FlashTask::Setup()
{
    FlashSettings::instance = &settings;
    settings.Init();
}

void FlashTask::Init()
{
    SetState(FlashState::SelectAddress);
    RunLight::Off();
    SwitchScanTask::instance->Subscribe(FlashTask::OnToggle, this);
}

void FlashTask::Cleanup()
{
    SwitchScanTask::instance->Unsubscribe(this);
    RunLight::Off();
    DisplayRegister = 0;
}

void FlashTask::SetState(FlashState state)
{
    flashState  = state;
    entryAction = true;
}

void FlashTask::PlayConfirmationTone()
{
    BT201TransactionTask::instance->PlayTrack("SYSTEM", 2);
}

void FlashTask::Execute()
{
    switch (flashState)
    {
        case FlashState::SelectAddress:
            switches = SwitchRegister;
            DisplayRegister = switches;
            flashEntry = FlashSettings::instance->LookupByAddress(switches);

            if (flashEntry)
                RunLight::On();
            else
                RunLight::Off();
            break;

        case FlashState::SelectData:
            if (entryAction)
            {
                DisplayRegister = FlashSettings::instance->Get(flashEntry->name);
                switches = SwitchRegister;
                Serial.print("Flash value (R): "), Serial.println(DisplayRegister, BIN);
            }

            if (switches != SwitchRegister)
            {
                switches = SwitchRegister;
                DisplayRegister = switches;
            }

            if (switches >= flashEntry->minValue && switches <= flashEntry->maxValue)
                RunLight::On();
            else
                RunLight::Off();
            break;

        case FlashState::CommitData:
            if (switches >= flashEntry->minValue && switches <= flashEntry->maxValue)
            {
                FlashSettings::instance->Set(flashEntry->name, switches);
                Serial.print("Flash value (W): "), Serial.println(switches, BIN);
                PlayConfirmationTone();
            }
            RunLight::Off();
            SetState(FlashState::SelectAddress);
            break;

        case FlashState::SelectReset:
            if (entryAction)
            {
                RunLight::Off();
                DisplayRegister = 0xAAAA;
            }
            break;

        case FlashState::CommitReset:
            FlashSettings::instance->SetDefaults();
            PlayConfirmationTone();
            SetState(FlashState::SelectReset);
            break;

        case FlashState::SelectVUCal:
            if (entryAction)
                DisplayRegister = FlashSettings::instance->Get("dcOffset");
            break;

        case FlashState::Calibrate:
            if (VUMeterTask::instance->Calibrate())
                PlayConfirmationTone();
            SetState(FlashState::SelectVUCal);
            break;
    }

    entryAction = false;
}

void FlashTask::HandleSenseSwitch()
{
    switch (flashState)
    {
        case FlashState::SelectAddress:
            if (IsSenseSwitchActive(CONTINUE))
            {
                // Hidden provisioning functions.
                // Key in magic number and press CONTINUE to execute.
                // Remain in SelectAddress — power cycle required after each operation.
                if (SwitchRegister == 0177570)        // Switch register address     → RestoreFactoryDefaults()
                {
                    provisioning.RestoreFactoryDefaults();
                    PlayConfirmationTone();
                }
                else
                    SetState(FlashState::SelectReset);
            }
            else if (IsSenseSwitchActive(LOAD) && flashEntry)
                SetState(FlashState::SelectData);
            break;

        case FlashState::SelectData:
            if (IsSenseSwitchActive(DEPOSIT))
                SetState(FlashState::CommitData);
            else if (IsSenseSwitchActive(CONTINUE))
                SetState(FlashState::SelectReset);
            break;

        case FlashState::SelectReset:
            if (IsSenseSwitchActive(DEPOSIT))
                SetState(FlashState::CommitReset);
            else if (IsSenseSwitchActive(CONTINUE))
                SetState(FlashState::SelectVUCal);
            break;

        case FlashState::SelectVUCal:
            if (IsSenseSwitchActive(DEPOSIT))
                SetState(FlashState::Calibrate);
            else if (IsSenseSwitchActive(CONTINUE))
                SetState(FlashState::SelectAddress);
            break;

        default:
            break;
    }
}

void FlashTask::OnToggle(void* instance)
{
    static_cast<FlashTask*>(instance)->HandleSenseSwitch();
}
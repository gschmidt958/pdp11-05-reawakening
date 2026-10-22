#include "BT201Provisioning.h"

void BT201Provisioning::ConfigureUSBSerial(unsigned int RXPin, unsigned int TXPin)
{
  Serial2.begin(115200, SERIAL_8N1, RXPin, TXPin);
}

void BT201Provisioning::FactoryResetAndProvision(const char* btAudioName)
{
    Serial.println("FactoryResetAndProvision()");
    Serial.println("Restoring factory defaults...");
    SendCommand("AT+CW");
    
    Serial.println("Waiting for chip to settle...");
    delay(5000);
    FlushSerial(); // eat any power-on burst
    
    Serial.println("Setting chip modes...");
    SetChipModes(btAudioName);
    
    Serial.println("Power cycle required.");
}

// Configures the BT201's non-volatile settings for use by this application.
//
// Background: Most AT+C* commands write to flash and require a power cycle to take
// effect. Sending them all back-to-back appears unreliable; inserting a short delay
// between each command seems to help, likely giving the chip time to complete the
// flash write before the next command arrives. This has not been fully verified.
//

void BT201Provisioning::SetChipModes(const char* btAudioName)
{
  Serial.println("SetChipModes()");

  // Suppress audio feedback (tones and voice prompts on connect, etc.)
  MuteAudioPrompts();
  delay(100);

  // Minimize serial output during power up (only emits QM+09).
  // Use ChattyPowerUp() instead if you need to observe the startup sequence.
  QuietPowerUp();
  delay(100);

  // Set the Bluetooth audio device name as it appears to pairing clients.
  SetBTAudioName(btAudioName);
  delay(100);

  // Automatically enter BT mode when powering on.
  //PowerOnEntersBTMode();
  //delay(100);

  // Wait for explicit mode instruction on startup rather than auto-selecting.
  WaitForModeInstructionOn();
  delay(100);

  // Enable unsolicited return data so track completion events are reported.
  // This is the chip default, but set explicitly for clarity.
  AutoReturnDataOn();
  delay(100);

  // Play a single track rather than looping.
  SinglePlay();
  delay(100);

  // Set initial volume (0-30).
  SetVolume(15);
  delay(100);

  // Enable BT Background
  EnableBTBackground();
  delay(100);

}

void BT201Provisioning::RestoreFactoryDefaults()
{
  Serial.println("RestoreFactoryDefaults");
  SendCommand("AT+CW");
}

void BT201Provisioning::SetBTAudioName(const char* name)
{
  Serial.println("SetBTAudioName");
  char command[32];
  snprintf(command, sizeof(command), "AT+BD%s", name);
  SendCommand(command);
}

void BT201Provisioning::PowerOnEntersBTMode()
{
  Serial.println("PowerOnEntersBTMode");
  SendCommand("AT+CP00");
}

void BT201Provisioning::EnableBTBackground()
{
  Serial.println("EnableBTBackground");
  SendCommand("AT+CG01");
}

void BT201Provisioning::WaitForModeInstructionOn()
{
  Serial.println("WaitForModeInstructionOn");
  SendCommand("AT+CP01");
}

void BT201Provisioning::MuteAudioPrompts()
{
  Serial.println("MuteAudioPrompts");
  SendCommand("AT+CN00");
}

void BT201Provisioning::ChattyPowerUp()
{
  Serial.println("ChattyPowerUp");
  SendCommand("AT+C201");  // Outputs several segments
}

void BT201Provisioning::QuietPowerUp()
{
  Serial.println("QuietPowerUp");
  SendCommand("AT+C200");  // Only appears to output QM+09
}

void BT201Provisioning::AutoReturnDataOn()
{
  Serial.println("AutoReturnDataOn");
  SendCommand("AT+CR01");
}

void BT201Provisioning::AutoReturnDataOff()
{
  Serial.println("AutoReturnDataOff");
  SendCommand("AT+CR00");
}

void BT201Provisioning::SinglePlay()
{
  Serial.println("SinglePlay");
  SendCommand("AT+CJ01");
}

void BT201Provisioning::SetVolume(uint8_t level)
{
  if (level > 30) level = 30;

  char command[32];
  snprintf(command, sizeof(command), "AT+CA%02d", level);
  SendCommand(command);
}

int BT201Provisioning::QueryVolume()
{
    Serial2.println("AT+QA");
    lastByteTime = millis();
    String line = "";

    while (millis() - lastByteTime < timeoutMs)
    {
        while (Serial2.available())
        {
            char c = Serial2.read();
            lastByteTime = millis();
            if (c == '\r') continue;
            if (c == '\n')
            {
                Serial.println(line);
                if (line.startsWith("QA+"))
                {
                    int vol = line.substring(3).toInt();
                    Serial.print("Volume: ");
                    Serial.println(vol);
                    return vol;
                }
                line = "";
            }
            else if (isprint(c))
                line += c;
        }
    }

    Serial.println("[BT201] QueryVolume: timeout");
    return -1;
}

bool BT201Provisioning::IsProvisioned(const char* btAudioName)
{
    Serial2.println("AT+TD");
    lastByteTime = millis();
    String line = "";
    bool result = false;

    while (millis() - lastByteTime < timeoutMs)
    {
        while (Serial2.available())
        {
            char c = Serial2.read();
            lastByteTime = millis();
            if (c == '\r') continue;
            if (c == '\n')
            {
                Serial.println(line);
                if (line.startsWith("TD+"))
                    result = (line.substring(3) == btAudioName);
                line = "";
            }
            else if (isprint(c))
                line += c;
        }
    }
    Serial.println("[BT201] IsProvisioned: timeout");
    return result;
}

void BT201Provisioning::FlushSerial()
{
    lastByteTime = millis();
    String line = "";

    while (millis() - lastByteTime < timeoutMs)
    {
        while (Serial2.available())
        {
            char c = Serial2.read();
            lastByteTime = millis();
            if (c == '\r') continue;
            if (c == '\n')
            {
                if (line.length() > 0)
                {
                    Serial.println(line);
                    line = "";
                }
            }
            else if (isprint(c))
                line += c;
        }
    }
}

void BT201Provisioning::SendCommand(const char* command)
{
    Serial2.println(command);
    FlushSerial();
}
#include <Arduino.h>
#include <driver/adc.h>

#include "FlashTask.h"
#include "VUMeterTask.h"
#include "TaskSchedule.h"
#include "Registers.h"
#include "BT201TransactionTask.h"

uint16_t VUMeterTask::r2lVuBits[17] = {
  0b0000000000000000,
  0b0000000000000001,
  0b0000000000000011,
  0b0000000000000111,
  0b0000000000001111,
  0b0000000000011111,
  0b0000000000111111,
  0b0000000001111111,
  0b0000000011111111,
  0b0000000111111111,
  0b0000001111111111,
  0b0000011111111111,
  0b0000111111111111,
  0b0001111111111111,
  0b0011111111111111,
  0b0111111111111111,
  0b1111111111111111
};

uint16_t VUMeterTask::l2rVuBits[17] = {
  0b0000000000000000,
  0b1000000000000000,
  0b1100000000000000,
  0b1110000000000000,
  0b1111000000000000,
  0b1111100000000000,
  0b1111110000000000,
  0b1111111000000000,
  0b1111111100000000,
  0b1111111110000000,
  0b1111111111000000,
  0b1111111111100000,
  0b1111111111110000,
  0b1111111111111000,
  0b1111111111111100,
  0b1111111111111110,
  0b1111111111111111
};

uint16_t VUMeterTask::centerBits[17] = {
  0b0000000000000000,
  0b0000000110000000,
  0b0000000110000000,
  0b0000001111000000,
  0b0000001111000000,
  0b0000011111100000,
  0b0000011111100000,
  0b0000111111110000,
  0b0000111111110000,
  0b0001111111111000,
  0b0001111111111000,
  0b0011111111111100,
  0b0011111111111100,
  0b0111111111111110,
  0b0111111111111110,
  0b1111111111111111,
  0b1111111111111111
};

uint16_t VUMeterTask::edgeBits[17] = {
  0b0000000000000000,
  0b1000000000000001,
  0b1000000000000001,
  0b1100000000000011,
  0b1100000000000011,
  0b1110000000000111,
  0b1110000000000111,
  0b1111000000001111,
  0b1111000000001111,
  0b1111100000011111,
  0b1111100000011111,
  0b1111110000111111,
  0b1111110000111111,
  0b1111111001111111,
  0b1111111001111111,
  0b1111111111111111,
  0b1111111111111111
};

uint16_t VUMeterTask::l2rPeakBits[17] = {
  0b0000000000000000,
  0b1000000000000000,
  0b0100000000000000,
  0b0010000000000000,
  0b0001000000000000,
  0b0000100000000000,
  0b0000010000000000,
  0b0000001000000000,
  0b0000000100000000,
  0b0000000010000000,
  0b0000000001000000,
  0b0000000000100000,
  0b0000000000010000,
  0b0000000000001000,
  0b0000000000000100,
  0b0000000000000010,
  0b0000000000000001
};

uint16_t VUMeterTask::r2lPeakBits[17] = {
  0b0000000000000000,
  0b0000000000000001,
  0b0000000000000010,
  0b0000000000000100,
  0b0000000000001000,
  0b0000000000010000,
  0b0000000000100000,
  0b0000000001000000,
  0b0000000010000000,
  0b0000000100000000,
  0b0000001000000000,
  0b0000010000000000,
  0b0000100000000000,
  0b0001000000000000,
  0b0010000000000000,
  0b0100000000000000,
  0b1000000000000000
};

void VUMeterTask::SetStyle(Style style)
{
  this->style = style;
  Reset();

  switch (style)
  {
    case STYLE_R2LVU:
      levelBits        = r2lVuBits;
      peakBits         = r2lPeakBits;
      filterEnvelope   = true;
      kCharge          = 0.48f;
      kDischarge       = 0.32f;
      break;

    case STYLE_L2RVU:
      levelBits        = l2rVuBits;
      peakBits         = l2rPeakBits;
      filterEnvelope   = true;
      kCharge          = 0.48f;
      kDischarge       = 0.32f;
      break;

    case STYLE_CENTER:
      levelBits        = centerBits;
      filterEnvelope   = true;
      kCharge          = 0.20f;
      kDischarge       = 0.05f;
      break;

    case STYLE_EDGE:
      levelBits        = edgeBits;
      filterEnvelope   = true;
      kCharge          = 0.20f;
      kDischarge       = 0.05f;
      break;

    case STYLE_VOXC:
      levelBits        = centerBits;
      filterEnvelope   = true;
      kCharge          = 0.20f;
      kDischarge       = 0.05f;
      break;

    case STYLE_VOXE:
      levelBits        = edgeBits;
      filterEnvelope   = true;
      kCharge          = 0.20f;
      kDischarge       = 0.05f;
      break;

    case STYLE_B9:
      levelBits        = centerBits;
      filterEnvelope   = true;
      kCharge          = 0.10f;
      kDischarge       = 0.02f;
      break;

    case STYLE_BINARY:
      filterEnvelope    = true;
      kCharge           = 0.00005f;
      kDischarge        = 0.008f;
      break;
  }
}

VUMeterTask::Style VUMeterTask::GetStyle()
{
  return style;
}

bool VUMeterTask::IsVoiceStyle(Style style)
{
  return style == STYLE_VOXC || style == STYLE_VOXE || style == STYLE_B9;
}

void VUMeterTask::Reset()
{
  peakHoldLevel = 0;
  peakTimer = 0;
  charge = 0;
  envelope = 0;
  envelopeReady = false;
  DisplayRegister = 0;
  initEnv = true;
}

VUMeterTask* VUMeterTask::instance = nullptr;

void VUMeterTask::Setup()
{
  instance = this;
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

  style = STYLE_L2RVU;
}

void VUMeterTask::SetVUGain(uint16_t gainSetting)
{
  if (gainSetting >= 1 && gainSetting <= 100)
    vuGain = gainSetting;
}

uint8_t VUMeterTask::GetVUGain()
{
  return vuGain;
}

void VUMeterTask::SetBinaryGain(uint16_t gainSetting)
{
  if (gainSetting >= 1 && gainSetting <= 8)
  {
    binaryGainSetting = gainSetting;
    binaryGain = 1 << (6 + gainSetting); // 1=128,2=256,3=512,4=1024,5=2048,6=4096,7=8192,8=16384
  }
}

uint8_t VUMeterTask::GetBinaryGain()
{
  return binaryGainSetting;
}

void VUMeterTask::Init()
{
  dcOffset = FlashSettings::instance->Get("dcOffset");
  peakHoldDelay = FlashSettings::instance->Get("peakDecay");
  threshold = 0;
  envelopeReady = false;
  sampleCount = 0;
  Reset();
}

void VUMeterTask::Execute()
{
  if (!envelopeReady)
    return;

  if (initEnv)
  {
    charge    = envelope;
    threshold = envelope;
    initEnv   = false;
  }

  if (envelope >= charge)
    charge = charge * (1.0f - kCharge) + envelope * kCharge;
  else
    charge = charge * (1.0f - kDischarge);

  if (charge > threshold)
    threshold = threshold * 0.88f + charge * 0.12f;
  else
    threshold = threshold * 0.975f + charge * 0.025f;

  switch (style)
  {
    case STYLE_L2RVU:
    case STYLE_R2LVU:
    case STYLE_CENTER:
    case STYLE_EDGE:
      VUDisplay();
      break;
    case STYLE_VOXC:
    case STYLE_VOXE:
      VOXDisplay();
      break;
    case STYLE_B9:
      B9Display();
      break;
    case STYLE_BINARY:
      BinaryDisplay();
      break;
  }

  envelopeReady = false;
}

void VUMeterTask::VUDisplay()
{
  int level = (int)((charge / 10.0f) * vuGain / 100);

  if (level > 16) level = 16;
  if (level < 0) level = 0;

  DisplayRegister = levelBits[level];

  if (peakHoldDelay > 10)
  {
    if (style == STYLE_L2RVU || style == STYLE_R2LVU)
    {
      UpdatePeakHold(level);
      DisplayRegister |= peakBits[peakHoldLevel];
    }
  } 
}

void VUMeterTask::B9Display()
{
  DisplayRegister = (charge > threshold) ? 0xffff : 0x0000;
}

void VUMeterTask::VOXDisplay()
{
  float rel = charge - threshold;
  if (rel < 0.0f) rel = 0.0f;

  const float knee = 12.0f;
  float shaped = rel / (rel + knee);

  int level = (int)(shaped * 16.0f);
  if (level > 16) level = 16;

  DisplayRegister = levelBits[level];
}

void VUMeterTask::BinaryDisplay()
{
  int level = (int)(charge * binaryGain);

  if (level > 65535) level = 65535;

  DisplayRegister = (uint16_t)level;
}

void VUMeterTask::UpdatePeakHold(uint16_t level)
{
  if (level >= peakHoldLevel)
  {
    peakHoldLevel = level;
    peakTimer = peakHoldDelay;
  }
  else
  {
    if (peakTimer > 0)
      peakTimer--;
    else
    {
      if (peakHoldLevel > 0)
        peakHoldLevel--;
      peakTimer = peakHoldDelay;
    }
  }
}

// Called by hardware timer ISR at a fixed rate.
// Takes one ADC sample, computes envelope when window is full.
void IRAM_ATTR VUMeterTask::Sample()
{
  sampleBuffer[sampleCount++] = (int16_t)(adc1_get_raw(ADC1_CHANNEL_0) - dcOffset);

  if (sampleCount == WINDOW_SAMPLES)
  {
    if (filterEnvelope)
      FilteredEnvelope();
    else
      UnfilteredEnvelope();

    envelopeReady = true;
    sampleCount = 0;
  }
}

void IRAM_ATTR VUMeterTask::FilteredEnvelope()
{
  // Compute local DC as integer average
  int32_t sum = 0;
  for (int i = 0; i < WINDOW_SAMPLES; i++)
    sum += sampleBuffer[i];
  int16_t localDC = (int16_t)(sum / WINDOW_SAMPLES);

  uint32_t energySum = 0;
  int crosses = 0;
  int active = 0;

  for (int i = 0; i < WINDOW_SAMPLES; i++)
  {
    int16_t diff = sampleBuffer[i] - localDC;
    energySum += (uint32_t)((int32_t)diff * diff);

    if (abs(diff) > 45) active++;

    if (i > 0)
    {
      bool prevSign = (sampleBuffer[i-1] - localDC) > 0;
      bool currSign = (sampleBuffer[i] - localDC) > 0;
      if (prevSign != currSign)
        crosses++;
    }
  }

  if (crosses < 2 || active <= 7)
    envelope = 0;
  else
  {
    uint32_t meanSquare = energySum / WINDOW_SAMPLES;
    envelope = (meanSquare < 400) ? 0 : (uint16_t)sqrtf((float)meanSquare);
  }
}

void IRAM_ATTR VUMeterTask::UnfilteredEnvelope()
{
  uint32_t sum = 0;
  for (int i = 0; i < WINDOW_SAMPLES; i++)
    sum += (uint32_t)((int32_t)sampleBuffer[i] * sampleBuffer[i]);
  envelope = (uint16_t)sqrtf((float)(sum / WINDOW_SAMPLES));
}

bool VUMeterTask::Calibrate()
{
  int32_t sum = 0;
  for (int i = 0; i < 32; i++)
    sum += adc1_get_raw(ADC1_CHANNEL_0);
  int result = (int)((sum + 16) / 32);

  if (result < 100 || result > 3900)
    return false;

  FlashSettings::instance->Set("dcOffset", result);
  return true;
}
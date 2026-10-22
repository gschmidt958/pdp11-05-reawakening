#ifndef VUMETERTASK_H
#define VUMETERTASK_H

/*
 * VUMeterTask
 *
 * A real-time audio level display driven by ADC sampling.
 * Supports multiple display styles ranging from traditional
 * VU bar graphs to voice-activity and binary displays.
 * Runs as a system-level task, started and stopped
 * by application tasks that require it.
 *
 * AUDIO SAMPLING
 *   Samples are collected at a fixed rate set by the
 *   hardware timer in the main sketch. Into a sliding
 *   window buffer (WINDOW_SAMPLES). When a full window
 *   is ready, the envelope is computed and flagged for
 *   Execute() to consume on Core 1.
 *
 * ENVELOPE DETECTION
 *   Filtered mode computes RMS energy over the window,
 *   rejecting DC and low-activity signals using zero-
 *   crossing and active-sample counts. Unfiltered mode
 *   computes RMS directly against a fixed DC offset
 *   (stored in flash via CalibrateVUMeterTask).
 *
 * DISPLAY STYLES
 *   L2RVU / R2LVU  - Traditional bar graph with optional
 *                    peak hold indicator.
 *   CENTER / EDGE  - Symmetric bar expanding from center
 *                    or contracting from edges.
 *   VOXC / VOXE   - Voice activity display using an
 *                    adaptive threshold and soft knee.
 *   B9             - Binary on/off threshold display,
 *                    inspired by the B9 robot from
 *                    Lost in Space.
 *   BINARY         - Raw level mapped across all 16 bits.
 *   Style can be changed at any time via SetStyle().
 *   Voice styles are identified by IsVoiceStyle() for
 *   use by callers that wish to exclude them from
 *   user-selectable options.
 *
 * PEAK HOLD
 *   Active in L2RVU and R2LVU styles when peakHoldDelay
 *   (flash setting "peakDecay") exceeds 10. The peak
 *   indicator holds at the highest recent level then
 *   decays gradually.
 *
 * CALIBRATION
 *   DC offset is stored in flash ("dcOffset") and loaded
 *   at Init(). Use CalibrateVUMeterTask to measure and
 *   save the correct offset for the hardware.
 */

/*
 * CalibrateVUMeterTask
 *
 * Displays the current ADC DC offset.  When the function
 * is seleected, it measures the ADC DC offset and saves it
 * to flash for use by VUMeterTask. An out-of-range result is
 * rejected and not saved.
 *
 * The result is displayed and confirmed with an audio tone
 * on completion.
 */

class VUMeterTask : public Task
{
  public:
    VUMeterTask(const char* name, unsigned int period = 0)
      : Task(name, period)
    {
    }

    enum Style
    {
      STYLE_L2RVU,
      STYLE_R2LVU,
      STYLE_CENTER,
      STYLE_EDGE,
      STYLE_BINARY,
      STYLE_VOXC,
      STYLE_VOXE,
      STYLE_B9,

      STYLE_COUNT
    };

    void SetStyle(Style style);
    Style GetStyle();

    static bool IsVoiceStyle(Style style);

    static VUMeterTask* instance;

    void Setup() override;
    void Init() override;
    void Execute() override;

    void SetVUGain(uint16_t gainSetting);
    uint8_t GetVUGain();
    void SetBinaryGain(uint16_t gainSetting);
    uint8_t GetBinaryGain();

    // Called by hardware timer ISR to sample the audio
    void IRAM_ATTR Sample();

    // Determine DC offset and save to flash
    static bool Calibrate();

  private:

    // Display
    void VUDisplay();
    void B9Display();
    void VOXDisplay();
    void BinaryDisplay();

    Style style;

    // Display bitmaps
    static uint16_t l2rVuBits[17];
    static uint16_t r2lVuBits[17];
    static uint16_t centerBits[17];
    static uint16_t edgeBits[17];
    uint16_t* levelBits = l2rVuBits;

    static uint16_t l2rPeakBits[17];
    static uint16_t r2lPeakBits[17];
    uint16_t* peakBits = l2rPeakBits;

    // Support
    void Reset();
    bool initEnv;

    // Charge (for capacitor model)
    float charge;
    float kCharge = 0.6;
    float kDischarge = 0.4;

    // Display scaling
    // Gains are set for optimal SD card playback, external task(s) which use BT, should reduce them
    // according to the BT gains set in flash.
    uint8_t vuGain = 100;
    uint8_t binaryGainSetting = 7; // 8192
    uint32_t binaryGain = 1 << (6 + binaryGainSetting);

    // DC offset (bias)
    int dcOffset;

    // Adaptive threshold
    float threshold;

    // --- Sliding Window Sampler ---

    static const uint16_t WINDOW_SAMPLES = 25;  // This value keeps the sample buffer full without this
                                                // task having to wait when running at a 2ms period.

    // Envelope computation
    void IRAM_ATTR FilteredEnvelope();
    void IRAM_ATTR UnfilteredEnvelope();

    // Sample buffer
    int16_t sampleBuffer[WINDOW_SAMPLES] = {0};
    uint8_t sampleCount = 0;

    // Sampler state
    bool filterEnvelope = false;

    // Sampler output — volatile because written by timer ISR, read on Core 1
    // uint16_t is atomically read/written on ESP32; max RMS is ~2048, fits in uint16_t.
    volatile uint16_t envelope = 0;
    volatile bool     envelopeReady = false;

    // Peak hold feature
    uint16_t peakHoldDelay = 150;

    uint8_t  peakHoldLevel;
    uint16_t peakTimer;

    void UpdatePeakHold(uint16_t level);
};

#endif // VUMETERTASK_H
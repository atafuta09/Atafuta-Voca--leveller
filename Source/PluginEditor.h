#pragma once

#if __has_include(<JuceHeader.h>)
 #include <JuceHeader.h>
#else
 #include <juce_gui_basics/juce_gui_basics.h>
 #include <juce_audio_processors/juce_audio_processors.h>
#endif

#include "PluginProcessor.h"
#include <vector>

// ==============================================================================
/**
 * リアルタイム波形・ゲイン補正・レンジ幅描画カスタムComponent
 */
class WaveformVisualizerComponent : public juce::Component
{
public:
    WaveformVisualizerComponent();
    ~WaveformVisualizerComponent() override = default;

    void pushData (const VisualDataPoint* points, int numPoints);
    void setVisualParams (float targetDb, float rangeDb);
    void clear();
    void setGuiEnabled (bool enabled);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    static constexpr int maxHistoryPoints = 400; // 描画履歴ポイント数
    std::vector<VisualDataPoint> history;
    int writeIndex = 0;
    bool bufferWrapped = false;
    bool guiEnabled = true;

    float currentTargetDb = -12.0f;
    float currentRangeDb  = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformVisualizerComponent)
};

// ==============================================================================
/**
 * 右端に配置するスリムなステレオメーターComponent (Input & Output: Peak / RMS / VU)
 */
class SlimMeterComponent : public juce::Component
{
public:
    SlimMeterComponent();
    ~SlimMeterComponent() override = default;

    void updateLevels (float inLinear, float outLinear, int mode);
    void setGuiEnabled (bool enabled);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    int   currentMeterMode = 0; // 0: Peak, 1: RMS, 2: VU
    float inputLevelDb     = -60.0f;
    float outputLevelDb    = -60.0f;
    float inputPeakDb      = -60.0f;
    float outputPeakDb     = -60.0f;
    int   inHoldTimer      = 0;
    int   outHoldTimer     = 0;
    bool  guiEnabled       = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlimMeterComponent)
};

// ==============================================================================
/**
 * AutoLeveler プラグインエディター (GUI)
 */
class AutoLevelerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    explicit AutoLevelerAudioProcessorEditor (AutoLevelerAudioProcessor&);
    ~AutoLevelerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;
    void visibilityChanged() override;

private:
    void updateTimerState();
    void updateSyncControlState();

    AutoLevelerAudioProcessor& audioProcessor;

    // メイン描画エリア（波形 ＆ 右端スリムメーター ＆ メーターモード選択）
    WaveformVisualizerComponent waveformComponent;
    SlimMeterComponent          slimMeterComponent;
    juce::ComboBox              meterModeBox;

    // フェーダー式スライダー (Input, Target, Output)
    juce::Slider inputGainSlider;
    juce::Label  inputGainLabel;

    juce::Slider targetLevelSlider;
    juce::Label  targetLevelLabel;

    juce::Slider outputGainSlider;
    juce::Label  outputGainLabel;

    // ノブ式スライダー (Range, Speed)
    juce::Slider rangeSlider;
    juce::Label  rangeLabel;

    juce::Slider speedSlider;
    juce::Label  speedLabel;
    juce::Label  attackReleaseLabel; // Attack / Release 表示

    // モード切替コンボボックス (Detection Mode, Timing Mode, Sync Speed)
    juce::ComboBox detectionModeBox;
    juce::Label    detectionModeLabel;

    juce::ComboBox timingModeBox;
    juce::Label    timingModeLabel;

    juce::ComboBox syncSpeedBox;
    juce::Label    syncSpeedLabel;

    // トグルスイッチ (Lookahead, Breath Filter, Sibilance Filter, GUI Display)
    juce::ToggleButton lookaheadButton;
    juce::ToggleButton breathFilterButton;
    juce::ToggleButton sibilanceFilterButton;
    juce::ToggleButton guiEnableButton;

    // APVTS アタッチメント
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   targetLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   rangeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   speedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> detectionModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> timingModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncSpeedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> meterModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   lookaheadAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   breathFilterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   sibilanceFilterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   guiEnableAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoLevelerAudioProcessorEditor)
};

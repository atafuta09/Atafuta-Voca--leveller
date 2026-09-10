#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// WaveformVisualizerComponent 実装
// ==============================================================================
WaveformVisualizerComponent::WaveformVisualizerComponent()
    : history (static_cast<size_t>(maxHistoryPoints))
{
    setOpaque (true);
}

void WaveformVisualizerComponent::pushData (const VisualDataPoint* points, int numPoints)
{
    if (!guiEnabled || points == nullptr || numPoints <= 0)
        return;

    for (int i = 0; i < numPoints; ++i)
    {
        history[static_cast<size_t>(writeIndex)] = points[i];
        writeIndex = (writeIndex + 1) % maxHistoryPoints;
        if (writeIndex == 0)
            bufferWrapped = true;
    }

    repaint();
}

void WaveformVisualizerComponent::setVisualParams (float targetDb, float rangeDb)
{
    if (std::abs (currentTargetDb - targetDb) > 0.05f || std::abs (currentRangeDb - rangeDb) > 0.05f)
    {
        currentTargetDb = targetDb;
        currentRangeDb  = rangeDb;
        if (guiEnabled)
            repaint();
    }
}

void WaveformVisualizerComponent::clear()
{
    std::fill (history.begin(), history.end(), VisualDataPoint{});
    writeIndex = 0;
    bufferWrapped = false;
    repaint();
}

void WaveformVisualizerComponent::setGuiEnabled (bool enabled)
{
    if (guiEnabled != enabled)
    {
        guiEnabled = enabled;
        repaint();
    }
}

void WaveformVisualizerComponent::resized()
{
}

void WaveformVisualizerComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // 1. 背景描画
    g.setGradientFill (juce::ColourGradient (
        juce::Colour (0x12, 0x14, 0x1c), bounds.getX(), bounds.getY(),
        juce::Colour (0x18, 0x1b, 0x24), bounds.getX(), bounds.getBottom(),
        false
    ));
    g.fillRect (bounds);

    g.setColour (juce::Colour (0x28, 0x2e, 0x3d));
    g.drawRect (bounds, 1.0f);

    if (!guiEnabled)
    {
        g.setColour (juce::Colour (0x64, 0x74, 0x8b));
        g.setFont (juce::FontOptions (15.0f));
        g.drawText ("DISPLAY PAUSED (CPU Saving Mode)", bounds.toNearestInt(), juce::Justification::centred, true);

        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Turn on 'GUI Display' to view real-time analysis.", 
                    bounds.toNearestInt().translated (0, 24), juce::Justification::centred, true);
        return;
    }

    const float top = bounds.getY() + 15.0f;
    const float bottom = bounds.getBottom() - 15.0f;
    const float height = bottom - top;
    const float width = bounds.getWidth() - 52.0f;

    // 2. dBグリッド目盛りの描画
    const float gainMinDb = -24.0f;
    const float gainMaxDb = 18.0f;

    auto dbToY = [top, bottom, height, gainMinDb, gainMaxDb](float db) -> float
    {
        const float norm = (db - gainMinDb) / (gainMaxDb - gainMinDb);
        return bottom - (norm * height);
    };

    const float gridDbs[] = { 12.0f, 0.0f, -6.0f, -12.0f, -24.0f };
    g.setFont (juce::FontOptions (11.0f));

    for (float db : gridDbs)
    {
        const float y = dbToY (db);
        if (std::abs (db) < 0.01f)
            g.setColour (juce::Colour (0x4b, 0x55, 0x63).withAlpha (0.75f));
        else
            g.setColour (juce::Colour (0x23, 0x29, 0x36).withAlpha (0.6f));

        g.drawHorizontalLine (juce::roundToInt (y), bounds.getX() + 10.0f, bounds.getX() + width);

        g.setColour (juce::Colour (0x94, 0xa3, 0xb8));
        juce::String labelText = (db > 0.0f ? "+" : "") + juce::String (static_cast<int>(db)) + " dB";
        g.drawText (labelText, juce::roundToInt (bounds.getX() + width + 5.0f), juce::roundToInt (y - 7.0f), 45, 14, juce::Justification::centredLeft);
    }

    // 3. RMSレベルのdBマッピング関数（-42 dB 〜 0 dB）
    auto rmsToY = [top, bottom, height](float rms) -> float
    {
        if (rms < 0.0005f)
            return bottom;
        const float db = juce::Decibels::gainToDecibels (rms, -60.0f);
        const float norm = juce::jlimit (0.0f, 1.0f, (db + 42.0f) / 42.0f);
        return bottom - (norm * height * 0.94f);
    };

    // 4. レンジ幅（Range）およびターゲットレベルの視覚化（薄く帯状に表示）
    {
        const float rangeTopDb    = juce::jlimit (-42.0f, 0.0f, currentTargetDb + currentRangeDb);
        const float rangeBottomDb = juce::jlimit (-42.0f, 0.0f, currentTargetDb - currentRangeDb);

        const float rangeTopY    = rmsToY (juce::Decibels::decibelsToGain (rangeTopDb));
        const float rangeBottomY = rmsToY (juce::Decibels::decibelsToGain (rangeBottomDb));
        const float targetY      = rmsToY (juce::Decibels::decibelsToGain (currentTargetDb));

        // 薄いレンジ帯域のハイライト
        const float bandHeight = juce::jmax (2.0f, rangeBottomY - rangeTopY);
        g.setColour (juce::Colour (0xf5, 0x9e, 0x0b).withAlpha (0.08f));
        g.fillRect (bounds.getX() + 1.0f, rangeTopY, width - 1.0f, bandHeight);

        // レンジ境界の破線
        float dashPattern[] = { 4.0f, 4.0f };
        g.setColour (juce::Colour (0xf5, 0x9e, 0x0b).withAlpha (0.35f));
        g.drawDashedLine (juce::Line<float> (bounds.getX(), rangeTopY, bounds.getX() + width, rangeTopY), dashPattern, 2, 1.0f);
        g.drawDashedLine (juce::Line<float> (bounds.getX(), rangeBottomY, bounds.getX() + width, rangeBottomY), dashPattern, 2, 1.0f);

        // ターゲットレベルの基準ライン（シアン）
        g.setColour (juce::Colour (0x06, 0xb6, 0xd4).withAlpha (0.75f));
        g.drawLine (bounds.getX(), targetY, bounds.getX() + width, targetY, 1.5f);

        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.setColour (juce::Colour (0x06, 0xb6, 0xd4).withAlpha (0.9f));
        g.drawText ("TARGET " + juce::String (static_cast<int>(currentTargetDb)) + "dB", 
                    juce::roundToInt (bounds.getX() + width - 95.0f), juce::roundToInt (targetY - 14.0f), 90, 12, juce::Justification::centredRight);
    }

    // 5. 波形データの巡回描画
    const int availablePoints = bufferWrapped ? maxHistoryPoints : writeIndex;
    if (availablePoints < 2)
        return;

    const float xStep = width / static_cast<float>(maxHistoryPoints - 1);

    juce::Path inputWavePath;
    juce::Path outputWavePath;
    juce::Path gainLinePath;

    inputWavePath.startNewSubPath (bounds.getX(), bottom);
    outputWavePath.startNewSubPath (bounds.getX(), bottom);

    bool firstGainPoint = true;

    for (int i = 0; i < maxHistoryPoints; ++i)
    {
        const float x = bounds.getX() + (static_cast<float>(i) * xStep);

        int sampleIndex = 0;
        if (bufferWrapped)
            sampleIndex = (writeIndex + i) % maxHistoryPoints;
        else
        {
            sampleIndex = i - (maxHistoryPoints - writeIndex);
            if (sampleIndex < 0)
                continue;
        }

        const auto& pt = history[static_cast<size_t>(sampleIndex)];

        const float inY  = rmsToY (pt.inputRms);
        const float outY = rmsToY (pt.outputRms);

        inputWavePath.lineTo (x, inY);
        outputWavePath.lineTo (x, outY);

        const float gainY = juce::jlimit (top, bottom, dbToY (pt.gainChangeDb));
        if (firstGainPoint)
        {
            gainLinePath.startNewSubPath (x, gainY);
            firstGainPoint = false;
        }
        else
        {
            gainLinePath.lineTo (x, gainY);
        }
    }

    // 入力波形塗りつぶし（スレートブルー）
    inputWavePath.lineTo (bounds.getX() + width, bottom);
    inputWavePath.closeSubPath();
    g.setColour (juce::Colour (0x3b, 0x82, 0xf6).withAlpha (0.32f));
    g.fillPath (inputWavePath);

    // 出力波形塗りつぶし（ネオンシアン）
    outputWavePath.lineTo (bounds.getX() + width, bottom);
    outputWavePath.closeSubPath();
    g.setColour (juce::Colour (0x06, 0xb6, 0xd4).withAlpha (0.50f));
    g.fillPath (outputWavePath);

    g.setColour (juce::Colour (0x22, 0xd3, 0xee).withAlpha (0.85f));
    g.strokePath (outputWavePath, juce::PathStrokeType (1.5f));

    // ゲイン変化の軌跡ライン（アンバー）
    g.setColour (juce::Colour (0xf5, 0x9e, 0x0b));
    g.strokePath (gainLinePath, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 6. 凡例（HUD Legend）
    g.setFont (juce::FontOptions (11.0f));
    const float legendY = bounds.getY() + 7.0f;

    g.setColour (juce::Colour (0x3b, 0x82, 0xf6));
    g.fillRect (bounds.getX() + 12.0f, legendY + 3.0f, 8.0f, 8.0f);
    g.setColour (juce::Colour (0x94, 0xa3, 0xb8));
    g.drawText ("Input RMS", juce::roundToInt (bounds.getX() + 24.0f), juce::roundToInt (legendY), 65, 14, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0x06, 0xb6, 0xd4));
    g.fillRect (bounds.getX() + 98.0f, legendY + 3.0f, 8.0f, 8.0f);
    g.setColour (juce::Colour (0x94, 0xa3, 0xb8));
    g.drawText ("Output RMS", juce::roundToInt (bounds.getX() + 110.0f), juce::roundToInt (legendY), 70, 14, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xf5, 0x9e, 0x0b));
    g.fillRect (bounds.getX() + 190.0f, legendY + 3.0f, 8.0f, 8.0f);
    g.setColour (juce::Colour (0x94, 0xa3, 0xb8));
    g.drawText ("Gain Ride", juce::roundToInt (bounds.getX() + 202.0f), juce::roundToInt (legendY), 65, 14, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xf5, 0x9e, 0x0b).withAlpha (0.4f));
    g.drawRect (bounds.getX() + 276.0f, legendY + 3.0f, 14.0f, 8.0f, 1.0f);
    g.setColour (juce::Colour (0x94, 0xa3, 0xb8));
    g.drawText ("Range (±" + juce::String (currentRangeDb, 1) + "dB)", juce::roundToInt (bounds.getX() + 294.0f), juce::roundToInt (legendY), 95, 14, juce::Justification::centredLeft);
}

// ==============================================================================
// SlimMeterComponent 実装 (右端スリムピークメーター)
// ==============================================================================
SlimMeterComponent::SlimMeterComponent()
{
    setOpaque (true);
}

void SlimMeterComponent::updateLevels (float inLinear, float outLinear, int mode)
{
    if (!guiEnabled)
        return;

    currentMeterMode = mode;
    const float curInDb  = juce::Decibels::gainToDecibels (inLinear,  -60.0f);
    const float curOutDb = juce::Decibels::gainToDecibels (outLinear, -60.0f);

    if (mode == 0)
    {
        // --- PEAK モード: 瞬間アタック & 高速ディケイ ---
        constexpr float decayRate = 1.6f;
        inputLevelDb  = juce::jmax (curInDb,  inputLevelDb - decayRate);
        outputLevelDb = juce::jmax (curOutDb, outputLevelDb - decayRate);

        // ピークホールド (約45フレーム = 0.75秒保持)
        if (curInDb >= inputPeakDb)
        {
            inputPeakDb = curInDb;
            inHoldTimer = 45;
        }
        else if (inHoldTimer > 0)
            inHoldTimer--;
        else
            inputPeakDb = juce::jmax (inputLevelDb, inputPeakDb - 0.7f);

        if (curOutDb >= outputPeakDb)
        {
            outputPeakDb = curOutDb;
            outHoldTimer = 45;
        }
        else if (outHoldTimer > 0)
            outHoldTimer--;
        else
            outputPeakDb = juce::jmax (outputLevelDb, outputPeakDb - 0.7f);
    }
    else if (mode == 1)
    {
        // --- RMS モード: なめらかな音量感追従 ---
        constexpr float decayRate = 1.2f;
        inputLevelDb  = juce::jmax (curInDb,  inputLevelDb - decayRate);
        outputLevelDb = juce::jmax (curOutDb, outputLevelDb - decayRate);
        inputPeakDb   = inputLevelDb;
        outputPeakDb  = outputLevelDb;
    }
    else
    {
        // --- VU モード: 300ms標準VUバリスティクス (0 VU = -18 dBFS) ---
        constexpr float decayRate = 0.9f;
        inputLevelDb  = juce::jmax (curInDb,  inputLevelDb - decayRate);
        outputLevelDb = juce::jmax (curOutDb, outputLevelDb - decayRate);
        inputPeakDb   = inputLevelDb;
        outputPeakDb  = outputLevelDb;
    }

    repaint();
}

void SlimMeterComponent::setGuiEnabled (bool enabled)
{
    if (guiEnabled != enabled)
    {
        guiEnabled = enabled;
        repaint();
    }
}

void SlimMeterComponent::resized()
{
}

void SlimMeterComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // 背景
    g.setColour (juce::Colour (0x12, 0x14, 0x1c));
    g.fillRect (bounds);

    g.setColour (juce::Colour (0x28, 0x2e, 0x3d));
    g.drawRect (bounds, 1.0f);

    if (!guiEnabled)
        return;

    const float meterTop    = bounds.getY() + 18.0f;
    const float meterBottom = bounds.getBottom() - 14.0f;
    const float meterHeight = meterBottom - meterTop;

    const float barWidth = 9.0f;
    const float inBarX   = bounds.getX() + 5.0f;
    const float outBarX  = bounds.getX() + 19.0f;

    // トラック背景
    g.setColour (juce::Colour (0x1e, 0x22, 0x2e));
    g.fillRect (inBarX, meterTop, barWidth, meterHeight);
    g.fillRect (outBarX, meterTop, barWidth, meterHeight);

    if (currentMeterMode == 2)
    {
        // ================= VU モード描画 (0 VU = -18 dBFS, -20 VU ~ +3 VU) =================
        auto vuToY = [meterTop, meterBottom, meterHeight](float dbFs) -> float
        {
            const float vu = dbFs + 18.0f; // 0 VU = -18 dBFS
            const float norm = juce::jlimit (0.0f, 1.0f, (vu + 20.0f) / 23.0f);
            return meterBottom - (norm * meterHeight);
        };

        const float zeroVuY = vuToY (-18.0f);

        // IN バー (VU)
        const float inY = vuToY (inputLevelDb);
        if (inY < meterBottom)
        {
            const float subZeroHeight = meterBottom - std::max (zeroVuY, inY);
            g.setColour (juce::Colour (0xfb, 0xbf, 0x24)); // ウォームアンバー
            g.fillRect (inBarX, std::max (zeroVuY, inY), barWidth, subZeroHeight);

            if (inY < zeroVuY)
            {
                g.setColour (juce::Colour (0xef, 0x44, 0x44)); // 0 VU 超過レッド
                g.fillRect (inBarX, inY, barWidth, zeroVuY - inY);
            }
        }

        // OUT バー (VU)
        const float outY = vuToY (outputLevelDb);
        if (outY < meterBottom)
        {
            const float subZeroHeight = meterBottom - std::max (zeroVuY, outY);
            g.setColour (juce::Colour (0xfb, 0xbf, 0x24));
            g.fillRect (outBarX, std::max (zeroVuY, outY), barWidth, subZeroHeight);

            if (outY < zeroVuY)
            {
                g.setColour (juce::Colour (0xef, 0x44, 0x44));
                g.fillRect (outBarX, outY, barWidth, zeroVuY - outY);
            }
        }

        // 0 VU 基準線
        g.setColour (juce::Colour (0xef, 0x44, 0x44));
        g.drawHorizontalLine (juce::roundToInt (zeroVuY), inBarX, outBarX + barWidth);

        // VU 目盛り (+3, 0, -5, -10, -20)
        g.setFont (juce::FontOptions (8.0f));
        const float vuMarks[] = { 3.0f, 0.0f, -5.0f, -10.0f, -20.0f };
        for (float v : vuMarks)
        {
            const float y = vuToY (v - 18.0f);
            g.setColour (v >= 0.0f ? juce::Colour (0xef, 0x44, 0x44) : juce::Colour (0x94, 0xa3, 0xb8));
            juce::String s = (v > 0.0f ? "+" : "") + juce::String (static_cast<int>(v));
            g.drawText (s, juce::roundToInt (outBarX + barWidth + 2.0f), juce::roundToInt (y - 5.0f), 20, 10, juce::Justification::centredLeft);
        }
    }
    else
    {
        // ================= PEAK (0) & RMS (1) モード描画 (-48 dBFS ~ 0 dBFS) =================
        auto dbToY = [meterTop, meterBottom, meterHeight](float db) -> float
        {
            const float norm = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 54.0f);
            return meterBottom - (norm * meterHeight);
        };

        const float inY = dbToY (inputLevelDb);
        if (inY < meterBottom)
        {
            if (currentMeterMode == 0) // Peak
            {
                g.setGradientFill (juce::ColourGradient (
                    juce::Colour (0x60, 0xa5, 0xfa), inBarX, meterTop,
                    juce::Colour (0x25, 0x63, 0xeb), inBarX, meterBottom,
                    false));
            }
            else // RMS
            {
                g.setGradientFill (juce::ColourGradient (
                    juce::Colour (0x38, 0xbd, 0xf8), inBarX, meterTop,
                    juce::Colour (0x02, 0x84, 0xc7), inBarX, meterBottom,
                    false));
            }
            g.fillRect (inBarX, inY, barWidth, meterBottom - inY);
        }

        const float outY = dbToY (outputLevelDb);
        if (outY < meterBottom)
        {
            if (currentMeterMode == 0) // Peak
            {
                g.setGradientFill (juce::ColourGradient (
                    juce::Colour (0x22, 0xd3, 0xee), outBarX, meterTop,
                    juce::Colour (0x08, 0x91, 0xb2), outBarX, meterBottom,
                    false));
            }
            else // RMS
            {
                g.setGradientFill (juce::ColourGradient (
                    juce::Colour (0x2d, 0xd4, 0xbf), outBarX, meterTop,
                    juce::Colour (0x0d, 0x94, 0x88), outBarX, meterBottom,
                    false));
            }
            g.fillRect (outBarX, outY, barWidth, meterBottom - outY);
        }

        // ピークホールド (Peakモード時)
        if (currentMeterMode == 0)
        {
            const float inPeakY = dbToY (inputPeakDb);
            if (inPeakY < meterBottom)
            {
                g.setColour (juce::Colour (0x93, 0xc5, 0xfd));
                g.drawHorizontalLine (juce::roundToInt (inPeakY), inBarX, inBarX + barWidth);
            }

            const float outPeakY = dbToY (outputPeakDb);
            if (outPeakY < meterBottom)
            {
                g.setColour (juce::Colour (0xa5, 0xf3, 0xfc));
                g.drawHorizontalLine (juce::roundToInt (outPeakY), outBarX, outBarX + barWidth);
            }
        }

        // 0dB 目盛り線
        const float zeroY = dbToY (0.0f);
        g.setColour (juce::Colour (0xef, 0x44, 0x44).withAlpha (0.7f));
        g.drawHorizontalLine (juce::roundToInt (zeroY), inBarX, outBarX + barWidth);

        // 目盛り
        g.setFont (juce::FontOptions (8.0f));
        const float markDbs[] = { 0.0f, -12.0f, -24.0f, -36.0f };
        for (float m : markDbs)
        {
            const float y = dbToY (m);
            g.setColour (juce::Colour (0x64, 0x74, 0x8b));
            g.drawText (juce::String (static_cast<int>(m)), juce::roundToInt (outBarX + barWidth + 2.0f), juce::roundToInt (y - 5.0f), 20, 10, juce::Justification::centredLeft);
        }
    }

    // ラベル (IN / OUT)
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.setColour (juce::Colour (0x94, 0xa3, 0xb8));
    g.drawText ("IN", juce::roundToInt (inBarX - 1.0f), juce::roundToInt (bounds.getY() + 3.0f), juce::roundToInt (barWidth + 2.0f), 12, juce::Justification::centred);
    g.drawText ("OUT", juce::roundToInt (outBarX - 3.0f), juce::roundToInt (bounds.getY() + 3.0f), juce::roundToInt (barWidth + 6.0f), 12, juce::Justification::centred);
}

// ==============================================================================
// AutoLevelerAudioProcessorEditor 実装
// ==============================================================================
AutoLevelerAudioProcessorEditor::AutoLevelerAudioProcessorEditor (AutoLevelerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // --- 1. Input Gain フェーダー ---
    inputGainSlider.setSliderStyle (juce::Slider::LinearVertical);
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
    inputGainSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0x3b, 0x82, 0xf6));
    inputGainSlider.setColour (juce::Slider::trackColourId, juce::Colour (0x1e, 0x29, 0x3b));
    inputGainSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    inputGainSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    addAndMakeVisible (inputGainSlider);

    inputGainLabel.setText ("INPUT", juce::dontSendNotification);
    inputGainLabel.setJustificationType (juce::Justification::centred);
    inputGainLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    inputGainLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    addAndMakeVisible (inputGainLabel);

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::inputGain, inputGainSlider);

    // --- 2. Target Level フェーダー ---
    targetLevelSlider.setSliderStyle (juce::Slider::LinearVertical);
    targetLevelSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
    targetLevelSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0x06, 0xb6, 0xd4));
    targetLevelSlider.setColour (juce::Slider::trackColourId, juce::Colour (0x1e, 0x29, 0x3b));
    targetLevelSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    targetLevelSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    addAndMakeVisible (targetLevelSlider);

    targetLevelLabel.setText ("TARGET", juce::dontSendNotification);
    targetLevelLabel.setJustificationType (juce::Justification::centred);
    targetLevelLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    targetLevelLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    addAndMakeVisible (targetLevelLabel);

    targetLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::targetLevel, targetLevelSlider);

    // --- 3. Range ノブ (0.5 dB刻み) ---
    rangeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    rangeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
    rangeSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xf5, 0x9e, 0x0b));
    rangeSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    rangeSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    rangeSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    rangeSlider.setRange (0.0, 24.0, 0.5);
    addAndMakeVisible (rangeSlider);

    rangeLabel.setText ("RANGE (±dB)", juce::dontSendNotification);
    rangeLabel.setJustificationType (juce::Justification::centred);
    rangeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    rangeLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    addAndMakeVisible (rangeLabel);

    rangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::range, rangeSlider);

    // --- 4. Speed ノブ & Attack/Release 表示 ---
    speedSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    speedSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
    speedSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0x10, 0xb9, 0x81));
    speedSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    speedSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    speedSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    addAndMakeVisible (speedSlider);

    speedLabel.setText ("SPEED", juce::dontSendNotification);
    speedLabel.setJustificationType (juce::Justification::centred);
    speedLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    speedLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    addAndMakeVisible (speedLabel);

    attackReleaseLabel.setFont (juce::FontOptions (10.0f));
    attackReleaseLabel.setJustificationType (juce::Justification::centred);
    attackReleaseLabel.setColour (juce::Label::textColourId, juce::Colour (0x10, 0xb9, 0x81));
    addAndMakeVisible (attackReleaseLabel);

    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::speed, speedSlider);

    // --- 5. Output Gain フェーダー ---
    outputGainSlider.setSliderStyle (juce::Slider::LinearVertical);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
    outputGainSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xa8, 0x55, 0xf7));
    outputGainSlider.setColour (juce::Slider::trackColourId, juce::Colour (0x1e, 0x29, 0x3b));
    outputGainSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    outputGainSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    addAndMakeVisible (outputGainSlider);

    outputGainLabel.setText ("OUTPUT", juce::dontSendNotification);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    outputGainLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    outputGainLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    addAndMakeVisible (outputGainLabel);

    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::outputGain, outputGainSlider);

    // --- 6. モード切替セレクター (Detection Mode, Timing Mode, BPM Speed) ---
    detectionModeBox.addItem ("RMS", 1);
    detectionModeBox.addItem ("Peak", 2);
    detectionModeBox.setSelectedId (1);
    addAndMakeVisible (detectionModeBox);

    detectionModeLabel.setText ("DETECTOR", juce::dontSendNotification);
    detectionModeLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    detectionModeLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    detectionModeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (detectionModeLabel);

    detectionModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::detectionMode, detectionModeBox);

    timingModeBox.addItem ("Free (ms)", 1);
    timingModeBox.addItem ("Sync (BPM)", 2);
    timingModeBox.setSelectedId (1);
    timingModeBox.onChange = [this]
    {
        updateSyncControlState();
    };
    addAndMakeVisible (timingModeBox);

    timingModeLabel.setText ("TIMING", juce::dontSendNotification);
    timingModeLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    timingModeLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    timingModeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (timingModeLabel);

    timingModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::timingMode, timingModeBox);

    syncSpeedBox.addItem ("Fast", 1);
    syncSpeedBox.addItem ("Mid",  2);
    syncSpeedBox.addItem ("Slow", 3);
    syncSpeedBox.setSelectedId (2);
    addAndMakeVisible (syncSpeedBox);

    syncSpeedLabel.setText ("BPM SPEED", juce::dontSendNotification);
    syncSpeedLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    syncSpeedLabel.setColour (juce::Label::textColourId, juce::Colour (0x94, 0xa3, 0xb8));
    syncSpeedLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (syncSpeedLabel);

    syncSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::syncSpeed, syncSpeedBox);

    // SpeedノブとSyncSpeed (Fast, Mid, Slow) の双方向同期
    speedSlider.onValueChange = [this]
    {
        const bool isSyncMode = (timingModeBox.getSelectedId() == 2);
        if (isSyncMode)
        {
            const double val = speedSlider.getValue();
            const int targetId = (val > 66.0) ? 1 : ((val < 34.0) ? 3 : 2); // 1: Fast, 2: Mid, 3: Slow
            if (syncSpeedBox.getSelectedId() != targetId)
                syncSpeedBox.setSelectedId (targetId, juce::sendNotification);
        }
    };

    syncSpeedBox.onChange = [this]
    {
        const bool isSyncMode = (timingModeBox.getSelectedId() == 2);
        if (isSyncMode)
        {
            const int id = syncSpeedBox.getSelectedId();
            const double targetVal = (id == 1) ? 100.0 : ((id == 3) ? 0.0 : 50.0);
            if (std::abs (speedSlider.getValue() - targetVal) > 5.0)
                speedSlider.setValue (targetVal, juce::dontSendNotification);
        }
    };

    // --- 7. メーターモード切替コンボボックス (Peak / RMS / VU) ---
    meterModeBox.addItem ("Peak", 1);
    meterModeBox.addItem ("RMS",  2);
    meterModeBox.addItem ("VU",   3);
    meterModeBox.setSelectedId (1);
    addAndMakeVisible (meterModeBox);

    meterModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::meterMode, meterModeBox);

    // --- 8. ヘッダートグルスイッチ群 ---
    lookaheadButton.setButtonText ("Lookahead (5ms)");
    lookaheadButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    lookaheadButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0x38, 0xbd, 0xf8));
    addAndMakeVisible (lookaheadButton);

    lookaheadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::lookaheadEnable, lookaheadButton);

    breathFilterButton.setButtonText ("Breath Filter");
    breathFilterButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    breathFilterButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0x34, 0xd3, 0x99));
    addAndMakeVisible (breathFilterButton);

    breathFilterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::breathFilter, breathFilterButton);

    sibilanceFilterButton.setButtonText ("Sibilance Filter");
    sibilanceFilterButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    sibilanceFilterButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xa7, 0x8b, 0xfa));
    addAndMakeVisible (sibilanceFilterButton);

    sibilanceFilterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::sibilanceFilter, sibilanceFilterButton);

    guiEnableButton.setButtonText ("GUI Display");
    guiEnableButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xe2, 0xe8, 0xf0));
    guiEnableButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0x10, 0xb9, 0x81));
    addAndMakeVisible (guiEnableButton);

    guiEnableButton.onClick = [this]
    {
        updateTimerState();
    };

    guiEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), ParameterIDs::guiEnable, guiEnableButton);

    // --- 9. 波形描画 & スリムメーター ---
    addAndMakeVisible (waveformComponent);
    addAndMakeVisible (slimMeterComponent);

    // ウィンドウサイズ設定 (幅880px, 高さ520px)
    setSize (880, 520);

    updateSyncControlState();
    updateTimerState();
}

AutoLevelerAudioProcessorEditor::~AutoLevelerAudioProcessorEditor()
{
    stopTimer();
}

void AutoLevelerAudioProcessorEditor::updateSyncControlState()
{
    const bool isSyncMode = (timingModeBox.getSelectedId() == 2);
    syncSpeedBox.setEnabled (isSyncMode);
    syncSpeedLabel.setColour (juce::Label::textColourId,
        isSyncMode ? juce::Colour (0x94, 0xa3, 0xb8) : juce::Colour (0x47, 0x55, 0x69));
}

void AutoLevelerAudioProcessorEditor::updateTimerState()
{
    const bool isGuiOn = guiEnableButton.getToggleState();
    const bool shouldRun = isShowing() && isGuiOn;

    if (shouldRun)
    {
        if (!isTimerRunning())
            startTimerHz (60);
    }
    else
    {
        if (isTimerRunning())
            stopTimer();
    }

    waveformComponent.setGuiEnabled (isGuiOn);
    slimMeterComponent.setGuiEnabled (isGuiOn);
}

void AutoLevelerAudioProcessorEditor::visibilityChanged()
{
    updateTimerState();
}

void AutoLevelerAudioProcessorEditor::timerCallback()
{
    if (!isShowing() || !guiEnableButton.getToggleState())
    {
        stopTimer();
        return;
    }

    // 1. ProcessorのFIFOから波形データを取得
    constexpr int maxReadPoints = 64;
    VisualDataPoint points[maxReadPoints];
    const int numRead = audioProcessor.readVisualData (points, maxReadPoints);
    if (numRead > 0)
    {
        waveformComponent.pushData (points, numRead);
    }

    // 2. ターゲットレベルとレンジ幅の視覚化更新
    const float curTarget = static_cast<float>(targetLevelSlider.getValue());
    const float curRange  = static_cast<float>(rangeSlider.getValue());
    waveformComponent.setVisualParams (curTarget, curRange);

    // 3. Attack / Release 値の更新表示 (Free vs BPM Sync 3段階: Fast, Mid, Slow)
    const bool isSyncMode = (timingModeBox.getSelectedId() == 2);
    const int syncSpeedIndex = syncSpeedBox.getSelectedId() - 1; // 0: Fast, 1: Mid, 2: Slow
    const auto timing = AutoLevelerAudioProcessor::calculateTiming (
        static_cast<float>(speedSlider.getValue()),
        isSyncMode,
        syncSpeedIndex,
        audioProcessor.getCurrentBpm());

    juce::String timingStr;
    if (isSyncMode)
    {
        timingStr = timing.modeName + ": Att " + timing.attackLabel + " | Rel " + timing.releaseLabel
                    + " @" + juce::String (juce::roundToInt (audioProcessor.getCurrentBpm())) + "BPM";
    }
    else
    {
        timingStr = "Att: " + timing.attackLabel + " | Rel: " + timing.releaseLabel;
    }
    attackReleaseLabel.setText (timingStr, juce::dontSendNotification);

    // 4. 右端スリムメーターの更新 (Peak / RMS / VU)
    const int activeMeterMode = meterModeBox.getSelectedId() - 1; // 0: Peak, 1: RMS, 2: VU
    float inVal  = 0.0f;
    float outVal = 0.0f;
    if (activeMeterMode == 1) // RMS
    {
        inVal  = audioProcessor.getLatestInputRms();
        outVal = audioProcessor.getLatestOutputRms();
    }
    else if (activeMeterMode == 2) // VU
    {
        inVal  = audioProcessor.getLatestInputVu();
        outVal = audioProcessor.getLatestOutputVu();
    }
    else // Peak (0)
    {
        inVal  = audioProcessor.getLatestInputPeak();
        outVal = audioProcessor.getLatestOutputPeak();
    }
    slimMeterComponent.updateLevels (inVal, outVal, activeMeterMode);
}

void AutoLevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0x0f, 0x11, 0x17));

    // ヘッダーバー
    const auto headerBounds = getLocalBounds().removeFromTop (46);
    g.setColour (juce::Colour (0x17, 0x1a, 0x23));
    g.fillRect (headerBounds);

    g.setColour (juce::Colour (0x26, 0x2b, 0x3a));
    g.drawHorizontalLine (46, 0.0f, static_cast<float>(getWidth()));

    // タイトル
    g.setColour (juce::Colour (0xf8, 0xfa, 0xfc));
    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawText ("AUTO LEVELER", 18, 0, 150, 46, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0x64, 0x74, 0x8b));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Vocal Dynamics Rider", 145, 0, 160, 46, juce::Justification::centredLeft);
}

void AutoLevelerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ヘッダーエリア
    auto headerArea = bounds.removeFromTop (46);
    guiEnableButton.setBounds       (headerArea.removeFromRight (105).reduced (4, 9));
    sibilanceFilterButton.setBounds (headerArea.removeFromRight (120).reduced (4, 9));
    breathFilterButton.setBounds    (headerArea.removeFromRight (110).reduced (4, 9));
    lookaheadButton.setBounds       (headerArea.removeFromRight (135).reduced (4, 9));

    // 下部コントロールパネル (高さ 165px)
    auto controlArea = bounds.removeFromBottom (165).reduced (16, 6);

    // 中央メインエリア (波形 + 右端スリムメーター ＆ メーターモード切替)
    auto mainVisualArea = bounds.reduced (16, 6);
    auto meterColumn = mainVisualArea.removeFromRight (54);
    meterModeBox.setBounds (meterColumn.removeFromTop (22).reduced (2, 1));
    meterColumn.removeFromTop (4);
    slimMeterComponent.setBounds (meterColumn);
    mainVisualArea.removeFromRight (6);
    waveformComponent.setBounds (mainVisualArea);

    // 下部コントロールの配置
    // [INPUT Fader] [TARGET Fader] [RANGE Knob] [SPEED Knob + Badge] [MODE / TIMING / SPEED] [OUTPUT Fader]
    const int totalWidth = controlArea.getWidth();
    const int faderW = totalWidth * 16 / 100;
    const int knobW  = totalWidth * 19 / 100;
    const int modeW  = totalWidth * 15 / 100;

    // 1. INPUT フェーダー
    auto inputArea = controlArea.removeFromLeft (faderW).reduced (6, 0);
    inputGainLabel.setBounds (inputArea.removeFromTop (18));
    inputGainSlider.setBounds (inputArea);

    // 2. TARGET フェーダー
    auto targetArea = controlArea.removeFromLeft (faderW).reduced (6, 0);
    targetLevelLabel.setBounds (targetArea.removeFromTop (18));
    targetLevelSlider.setBounds (targetArea);

    // 3. RANGE ノブ (0.5 dB刻み)
    auto rangeArea = controlArea.removeFromLeft (knobW).reduced (8, 0);
    rangeLabel.setBounds (rangeArea.removeFromTop (18));
    rangeSlider.setBounds (rangeArea);

    // 4. SPEED ノブ & Attack/Release バッジ
    auto speedArea = controlArea.removeFromLeft (knobW).reduced (6, 0);
    speedLabel.setBounds (speedArea.removeFromTop (18));
    attackReleaseLabel.setBounds (speedArea.removeFromBottom (18));
    speedSlider.setBounds (speedArea);

    // 5. モードセレクター (DETECTOR / TIMING / BPM SPEED)
    auto modeArea = controlArea.removeFromLeft (modeW).reduced (4, 2);
    const int secH = modeArea.getHeight() / 3;

    auto detSection = modeArea.removeFromTop (secH);
    detectionModeLabel.setBounds (detSection.removeFromTop (15));
    detectionModeBox.setBounds (detSection.reduced (2, 1));

    auto timingSection = modeArea.removeFromTop (secH);
    timingModeLabel.setBounds (timingSection.removeFromTop (15));
    timingModeBox.setBounds (timingSection.reduced (2, 1));

    auto syncSpeedSection = modeArea;
    syncSpeedLabel.setBounds (syncSpeedSection.removeFromTop (15));
    syncSpeedBox.setBounds (syncSpeedSection.reduced (2, 1));

    // 6. OUTPUT フェーダー
    auto outputArea = controlArea.reduced (6, 0);
    outputGainLabel.setBounds (outputArea.removeFromTop (18));
    outputGainSlider.setBounds (outputArea);
}

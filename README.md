# AutoLeveler (Atafuta Vocal Leveler)

JUCE / C++ で開発された、ボーカル特化型のオートレベラー VST3 / Standalone プラグインです。
（リファレンス: iZotope Nectar 4 Auto Level）

## 主な機能

- **Auto Gain Riding**: ターゲットレベルと許容レンジ（Range）に基づき、ボーカルの抑揚を自動で一定に整音
- **Drive Mode**: RMS駆動（自然で滑らかな音量追従） / Peak駆動（トランジェント重視の俊敏な追従）
- **Timing Mode**:
  - **Free**: アタック（2ms〜150ms）/ リリース（45ms〜800ms）をミリ秒で連続可変
  - **Sync**: ホストDAWのBPMに同期し、**Fast（1/64, 1/16） / Mid（1/32, 1/8） / Slow（1/16, 1/4）** の3段階音価で動作
- **Filters**:
  - **Lookahead (5ms)**: 5ms先読みディレイ（DAW自動レイテンシー補正対応、ON/OFF可能）
  - **Breath Filter**: 吹かれノイズ除去（150Hz HPF）と息継ぎ区間のゲイン暴走防止
  - **Sibilance Filter**: 歯擦音除去（4000Hz LPF）によりサ行での不自然なダッキングを防止
- **Visualizer & Meters**:
  - リアルタイム波形ディスプレイ（入力RMS、出力RMS、ゲイン変化軌跡、レンジ帯域表示）
  - 右端スリムメーター（**Peak / RMS / VU [0 VU = -18 dBFS]** 切替対応）

## 開発環境

- **言語**: C++20
- **フレームワーク**: JUCE 8 (CMake FetchContent)
- **ビルドツール**: CMake 3.22+, Visual Studio 2022/2026 (MSVC)
- **プラグイン形式**: VST3, Standalone

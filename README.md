# Atafuta09Leveler (Ver 1.04)

![Atafuta09Leveler](https://img.shields.io/badge/version-1.04-blue.svg)
![VST3](https://img.shields.io/badge/format-VST3-orange.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![JUCE8](https://img.shields.io/badge/JUCE-8.0.4-green.svg)

**Atafuta09Leveler** は、JUCE 8 / C++20 で開発されたプロ仕様のボーカル特化型オートレベラー（Vocal Dynamics Rider）VST3 プラグインです。  
コンプレッサーの潰れた質感を与えることなく、ボーカルの手動ボリュームオートメーション（手書きフェーダー操作）を完全に自動化し、オケに埋もれない自然で安定したボーカルトラックを瞬時に作成します。

---

## 主な特徴

- **Auto Dynamics Riding**: ターゲットレベルと許容補正幅（Range: 0〜15dB）に基づき、ボーカルの抑揚を滑らかに自動制御。
- **Modern Dark & White Mode**:
  - 実機ラック機材の質感を持つ **Dark Mode**
  - 高品位スタジオコンソールを模した高コントラストな **White Mode**（`COLOR` ボタンで瞬時切替）
- **Nectar 4 方式 統合型ターゲットフェーダー**: スライダー内部にリアルタイム入力音量が光り上がる直感的なGUI。
- **リアルタイム波形ビジュアライザー**: 入力波形、出力波形、およびゲイン補正レーザー軌跡を60fpsで滑らかにプロット。
- **充実のファクトリープリセット ＆ ユーザープリセット保存**:
  - `Default`、`Vocaloid`（ボカロ特化）、`Podcast`、`Aggressive Leveler` など全8種内蔵。
  - **`SAVE` ボタン** により、自作プリセットを `%APPDATA%` に安全に永続化保存可能。
- **ピークホールド付きマルチメーター**: Peak（1.5秒ホールドライン付）、RMS、VU-18（0 VU = -18 dBFS、レッドゾーン警告付）を切替可能。
- **プロセッシングフィルター**:
  - **Lookahead (5ms)**: 先読みバッファ（DAW遅延補正対応）によりアタックの頭潰れを完全防止。
  - **Breath Filter**: 吹かれノイズ除去（150Hz HPF）と息継ぎ区間の音量暴走を防止。
  - **Sibilance Filter**: 歯擦音除去（4000Hz LPF）によりサ行での不自然なダッキングを防止。
- **UIスケーリング**: 50% 〜 130%（10%刻み）でウィンドウサイズを自由に拡大縮小。

---

## ドキュメント一覧

- 📖 **[取扱説明書・機能解説 (MANUAL.md)](MANUAL.md)**: 各ノブやフェーダー、フィルターの詳細な役割と使い方。
- 📜 **[バージョン別 変更履歴 (CHANGELOG.md)](CHANGELOG.md)**: Ver 1.00 から Ver 1.04 までの開発作業と更新記録。

---

## 開発環境・ビルド仕様

- **言語**: C++20 / C++17
- **フレームワーク**: JUCE 8 (8.0.4)
- **ビルドツール**: CMake 3.22+, Visual Studio 2022/2026 (MSVC)
- **プラグイン形式**: VST3 (64-bit)
- **プラグイン表示名**: `Atafuta09Leveler`

---

## リンク

- 公式 X (Twitter): [@atafuta09](https://x.com/atafuta09)
- 公式 YouTube: [@atafuta09](https://www.youtube.com/@atafuta09)

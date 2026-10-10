# BASEMENT — Underground Melody Generator

US underground rap için melodi, akor ve 808 üreten bir VST3 / AU / Standalone plugin.
Bir stil seçiyorsun, **GENERATE**'e basıyorsun, beğendiğin loop'u direkt DAW'a sürüklüyorsun.

![ui](docs/ui.png)

## Stiller

| Stil | Karakter | Gam | Tempo |
|---|---|---|---|
| **Dark Trap** | Ürkütücü bell/piano, 808'e bol boşluk | Harmonic minor | ~145 |
| **Memphis Phonk** | Hipnotik 1 bar'lık loop'lar, alçak ve tozlu | Phrygian | ~130 |
| **Boom Bap** | Dorian m7 akorlar, sample-chop hissi (swing'i aç) | Dorian | ~88 |
| **Detroit / Flint** | Hızlı, gergin staccato piano koşuları ve stab'lar | Natural minor | ~100 |
| **Plugg** | Klasik plugg (MexikoDro ekolü): maj7 akorlar, yumuşak seyrek pentatonik lead | Major | ~150 |
| **Rage** | Synth lead, minor pentatonik, oktav atlamaları | Minor pentatonic | ~160 |
| **Drill** | Triplet karşı-melodiler, korku filmi harmonic minor | Harmonic minor | ~142 |
| **Angelic** | Yeni dalga underground (wegonebeok / Nettspend / OsamaSon havası): lydian maj9 loop'lar, tiz hipnotik 1 bar'lık synth'ler | Lydian | ~158 |
| **Ethereal Sample** | Soluk, sample-flip hissi (fakemink / Lucki havası): dorian m9 loop'lar, uzun puslu notalar — tape + reverb ekle | Dorian | ~150 |
| **Bells** | Tiz, tekrarlı 16'lık bell riff'leri ve oktav sekmeleri (TRGC'nin Yeat bell'leri gibi) | Minor pentatonic | ~160 |

## Kontroller

- **Style / Key / Scale / Bars / Octave** — Scale'i "Style Default" bırakırsan stilin kendi gamını kullanır.
- **Density** — nota yoğunluğu. Düşük = boşluklu hook, yüksek = koşu/arp.
- **Darkness** — gerilim. Yükseldikçe kromatik geçiş notaları artar, akorla çatışan notalar bilerek bırakılır, %66'nın üstünde minor gamların 2. derecesi bemolleşir (phrygian b2 — en "evil" aralık).
- **Swing** — 16'lıkları kaydırır (boom bap için %30–60 dene).
- **Humanize** — velocity ve zamanlama oynaması.
- **Melody / Chords / 808** — katmanları aç/kapat. Birini kapatmak diğerlerini değiştirmez.
- **Seed `<` `>`** — aynı ayarlarla seed'ler arasında gez; preset gibi. Seed + ayarlar = her zaman aynı melodi, proje kaydında saklanır.
- **Knob çevirmek melodiyi yeniden üretir ama seed'i korur** — yani Density'yi açınca aynı fikrin daha yoğun hâlini duyarsın.

## DAW'a aktarma

1. **Sürükle-bırak:** Alttaki `DRAG ALL` / `MELODY` / `CHORDS` / `808` kutularını playlist'e ya da piano roll'a sürükle.
   Dosya adı stil, ton, bpm ve seed'i içerir (`Basement_Rage_Fs_160bpm_4821_Melody.mid`).
2. **SAVE MIDI:** Tek dosyaya kaydet (her katman ayrı track: Melody / Chords / 808).
3. **Canlı MIDI out:** Plugin DAW transport'una senkron çalar ve MIDI çıkışı verir
   (kanal 1 = melodi, 2 = akor, 3 = 808). Kendi synth'ine route edebilirsin.
   - **FL Studio:** Wrapper → MIDI output port ayarla, synth'in input port'unu aynı yap.
   - **Ableton:** Yeni MIDI track → "MIDI From: Basement".
   - **Logic:** Instrument plugin'lerinden MIDI çıkışı almaz — orada sürükle-bırak kullan.
4. **Sound** açıkken dahili basit bir önizleme synth'i (FM bell + pad + sine 808) çalar. Sadece fikir dinlemek için;
   gerçek beat'te kendi sesini kullan. **PLAY** butonu DAW'sız (Standalone) önizleme içindir.

## FL Studio'da test

1. Bu repo'nun GitHub sayfasında **Actions** → en üstteki yeşil ✓ build → aşağıda **Artifacts** → `Basement-Windows` (Mac'te `Basement-macOS`) indir, zip'i aç.
2. `Basement.vst3` klasörünü kopyala:
   - Windows: `C:\Program Files\Common Files\VST3\`
   - Mac: `~/Library/Audio/Plug-Ins/VST3/` (sonra Terminal'de `xattr -cr ~/Library/Audio/Plug-Ins/VST3/Basement.vst3`)
3. FL: **Options → Manage plugins → Find installed plugins** (Start scan). Basement, **Generators** altında çıkar.
4. Channel Rack'te **+** → Basement. Pattern modunda Play'e bas: plugin FL'in temposuna ve transport'una senkron çalar,
   **Sound** açıkken dahili önizleme sesini duyarsın.
5. Stil seç, **GENERATE** / seed `<` `>` ile gez. Beğendiğinde:
   - `MELODY` kutusunu kendi synth'inin (Serum, Vital, FLEX...) channel'ına, **piano roll'un üstüne** sürükle → notalar o synth'e gelir.
   - `808` kutusunu 808 channel'ının piano roll'una, `CHORDS`'u pad/keys channel'ına aynı şekilde.
   - Ya da `DRAG ALL`'u playlist'e bırak; FL her katmanı ayrı kanal olarak import eder.
6. **Canlı routing (opsiyonel):** Basement penceresinde sol üstteki dişli → **MIDI output port** = örn. 1.
   Kendi synth'inde dişli → **MIDI input port** = 1. Sadece melodiyi göndermek için Chords ve 808'i kapat
   (Basement kanal 1-2-3'ü aynı porttan gönderir). Bu durumda Basement'in **Sound**'unu kapat.

Bir şey ters giderse (ses yok, plugin çıkmadı vs.) ekran görüntüsüyle söyle.

## Kurulum

Her push'ta GitHub Actions macOS (Universal: Apple Silicon + Intel), Windows ve Linux için derler.
Repo → **Actions** → son başarılı build → **Artifacts** bölümünden `Basement-macOS` / `Basement-Windows` indir.

- **Windows:** `Basement.vst3` klasörünü `C:\Program Files\Common Files\VST3\` içine kopyala.
- **macOS:** `Basement.vst3` → `~/Library/Audio/Plug-Ins/VST3/`, `Basement.component` → `~/Library/Audio/Plug-Ins/Components/`.
  Plugin imzasız olduğu için Terminal'de bir kere çalıştır:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/Basement.vst3 ~/Library/Audio/Plug-Ins/Components/Basement.component`

## Kaynaktan derleme

CMake ≥ 3.22 ve C++17 derleyici yeterli; JUCE otomatik indirilir.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
```

DAW'sız hızlı deneme için komut satırı aracı da var:

```bash
./build/basement-cli --style rage --key F# --bars 8 --seed 42 --darkness 0.8 -o rage.mid
```

## Proje yapısı

```
engine/    Melodi motoru — saf C++, JUCE'a bağımlı değil
  Styles.cpp     Her stilin "DNA"sı: gam, akor yürüyüşleri, ritim olasılıkları, motif formu
  Generator.cpp  Akor → motif (A/B/A' formları) → melodi → 808 → swing/humanize
  MidiWriter.cpp .mid yazıcı
plugin/    JUCE plugin: processor (host senkron MIDI), arayüz, önizleme synth'i
tools/     basement-cli
tests/     Motor testleri + gerçek zamanlı MIDI zamanlama testi
```

### Motor nasıl çalışıyor?

1. Stilin akor yürüyüşlerinden biri seçilir, akorlar voice-leading ile dizilir.
2. 1–2 bar'lık bir **motif** üretilir: ritim, stilin 16'lık olasılık ızgarasından (density ile ölçeklenir);
   perdeler gam içinde yürüyüşle (tekrar / adım / atlama / oktav), güçlü vuruşlarda akor notalarına çekilerek.
3. Motif bir **form** ile tekrarlanır (`AABA'`, `ABAB'`, `AAAB`...) — underground beat'lerdeki loop hissi buradan gelir.
   `A'` = motifin ilk yarısı aynı, ikinci yarısı yeni (call & response).
4. Tekrarlanan motif yeni akorla çatışırsa düzeltilir (Darkness yüksekse çatışma bilerek bırakılır).
5. 808 akor köklerini stilin bas ritmiyle çalar; son olarak swing ve humanize uygulanır.

Yeni stil eklemek için `engine/Styles.cpp`'ye bir blok eklemen yeterli.

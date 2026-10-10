#include "PluginEditor.h"

//==============================================================================
void WaveformView::setState (std::shared_ptr<const ChopState> s)
{
    state = std::move (s);
    repaint();
}

void WaveformView::setActiveSlice (int slice)
{
    if (slice != activeSlice)
    {
        activeSlice = slice;
        repaint();
    }
}

void WaveformView::setDropHighlight (bool on)
{
    dropHighlight = on;
    repaint();
}

void WaveformView::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (theme::panel);
    g.fillRoundedRectangle (area, 6.0f);

    if (dropHighlight)
    {
        g.setColour (theme::accent);
        g.drawRoundedRectangle (area.reduced (1.0f), 6.0f, 2.0f);
    }

    if (state == nullptr || state->sample == nullptr)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        g.drawText ("DROP A SAMPLE HERE", area.withTrimmedBottom (18.0f), juce::Justification::centred);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("wav / mp3 / aiff / flac  -  loops work best", area.withTrimmedTop (30.0f), juce::Justification::centred);
        return;
    }

    area.reduce (4.0f, 6.0f);
    const auto& s = *state->sample;
    const auto& slices = state->slices;
    const int len = s.getLength();
    const float mid = area.getCentreY();
    const float halfH = area.getHeight() * 0.5f;
    const int numPeaks = (int) s.peaks.size();

    auto xFor = [&] (int samplePos) { return area.getX() + area.getWidth() * (float) samplePos / (float) len; };

    // Slice backgrounds
    for (int i = 0; i < slices.numSlices(); ++i)
    {
        const float x0 = xFor (slices.starts[(size_t) i]), x1 = xFor (slices.sliceEnd (i, len));
        const auto c = theme::sliceColour (i);
        g.setColour (c.withAlpha (i == activeSlice ? 0.35f : 0.06f));
        g.fillRect (x0, area.getY(), x1 - x0, area.getHeight());
    }

    // Waveform
    const int w = (int) area.getWidth();
    for (int x = 0; x < w; ++x)
    {
        const int a = x * numPeaks / w, b = std::max (a + 1, (x + 1) * numPeaks / w);
        float p = 0;
        for (int k = a; k < b && k < numPeaks; ++k)
            p = std::max (p, s.peaks[(size_t) k]);

        const int samplePos = (int) ((int64_t) x * len / w);
        int slice = 0;
        while (slice + 1 < slices.numSlices() && slices.starts[(size_t) slice + 1] <= samplePos)
            ++slice;

        g.setColour (theme::sliceColour (slice).withAlpha (slice == activeSlice ? 1.0f : 0.7f));
        g.drawVerticalLine ((int) area.getX() + x, mid - p * halfH, mid + p * halfH);
    }

    // Slice markers + numbers
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    const bool showNumbers = area.getWidth() / (float) std::max (1, slices.numSlices()) > 14.0f;
    for (int i = 0; i < slices.numSlices(); ++i)
    {
        const float x = xFor (slices.starts[(size_t) i]);
        g.setColour (theme::background.withAlpha (0.8f));
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
        if (showNumbers)
        {
            g.setColour (theme::text.withAlpha (0.7f));
            g.drawText (juce::String (i + 1), juce::Rectangle<float> (x + 2.0f, area.getY(), 24.0f, 12.0f), juce::Justification::topLeft);
        }
    }
}

//==============================================================================
void ChopLane::setState (std::shared_ptr<const ChopState> s)
{
    state = std::move (s);
    repaint();
}

void ChopLane::setPlayhead (double beat)
{
    if (std::abs (beat - playhead) > 1e-4)
    {
        playhead = beat;
        repaint();
    }
}

void ChopLane::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (theme::panel);
    g.fillRoundedRectangle (area, 6.0f);
    area.reduce (6.0f, 6.0f);

    if (state == nullptr || state->sample == nullptr)
        return;

    const auto& p = state->pattern;
    const double beats = p.lengthBeats();
    auto xFor = [&] (double beat) { return area.getX() + (float) (beat / beats) * area.getWidth(); };

    for (int b = 0; b <= (int) beats; ++b)
    {
        g.setColour (b % 4 == 0 ? theme::grid.brighter (0.5f) : theme::grid);
        g.drawVerticalLine (juce::roundToInt (xFor (b)), area.getY(), area.getBottom());
    }

    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    for (auto& e : p.events)
    {
        auto r = juce::Rectangle<float> (xFor (e.start), area.getY(), xFor (e.start + e.length) - xFor (e.start), area.getHeight())
                     .reduced (0.5f, 2.0f);
        const auto c = theme::sliceColour (e.slice);
        g.setColour (c.withAlpha (0.35f + 0.5f * e.gain));
        g.fillRoundedRectangle (r, 2.5f);

        if (e.reverse)
        {
            g.setColour (theme::background.withAlpha (0.6f));
            g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);
        }

        if (r.getWidth() > 16.0f)
        {
            juce::String label (e.slice + 1);
            if (e.semitones > 0) label << "+";
            if (e.semitones != 0) label << juce::String (e.semitones);
            g.setColour (theme::background);
            g.drawText (label, r.reduced (3.0f, 0.0f), juce::Justification::topLeft);
        }
    }

    if (playhead >= 0.0)
    {
        g.setColour (theme::text);
        g.drawVerticalLine (juce::roundToInt (xFor (playhead)), area.getY() - 4.0f, area.getBottom() + 4.0f);
    }
}

//==============================================================================
DragChip::DragChip (BasementProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void DragChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool enabled = processor.hasSample();
    g.setColour (theme::accent.withAlpha (! enabled ? 0.08f : isMouseOver() ? 0.45f : 0.25f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (enabled ? theme::accent : theme::grid);
    g.drawRoundedRectangle (r, 5.0f, 1.5f);
    g.setColour (enabled ? theme::text : theme::textDim);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("DRAG WAV TO FL", r, juce::Justification::centred);
}

void DragChip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4 || ! processor.hasSample())
        return;

    dragging = true;
    const auto file = processor.renderToTempFile();
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                [safe = juce::Component::SafePointer<DragChip> (this)] {
                                                                    if (safe != nullptr)
                                                                        safe->dragging = false;
                                                                });
}

//==============================================================================
BasementEditor::BasementEditor (BasementProcessor& p)
    : AudioProcessorEditor (&p), proc (p), dragChip (p)
{
    setLookAndFeel (&lnf);

    title.setText ("BASEMENT", juce::dontSendNotification);
    title.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, theme::accent);
    addAndMakeVisible (title);

    subtitle.setText ("sample chopper", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (subtitle);

    description.setColour (juce::Label::textColourId, theme::textDim);
    description.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (description);

    sampleInfo.setColour (juce::Label::textColourId, theme::text);
    addAndMakeVisible (sampleInfo);

    loadButton.onClick = [this] { chooseSample(); };
    addAndMakeVisible (loadButton);
    addAndMakeVisible (waveform);

    setupCombo (style, "STYLE", "style");
    setupCombo (bars, "PATTERN BARS", "bars");
    setupCombo (chop, "CHOP SIZE", "chop");
    setupCombo (sampleBars, "SAMPLE BARS", "samplebars");

    setupKnob (pitch, "PITCH", "pitch");
    setupKnob (density, "DENSITY", "density");
    setupKnob (chaos, "CHAOS", "chaos");
    setupKnob (tone, "TONE", "tone");
    setupKnob (tempo, "TEMPO", "tempo");
    setupKnob (volume, "VOLUME", "volume");

    generateButton.setColour (juce::TextButton::buttonColourId, theme::accent);
    generateButton.onClick = [this] { proc.randomiseSeed(); };
    prevSeed.onClick = [this] { proc.nudgeSeed (-1); };
    nextSeed.onClick = [this] { proc.nudgeSeed (1); };
    playButton.onClick = [this] { proc.setInternalPlaying (! proc.isInternalPlaying()); };
    saveButton.onClick = [this] { saveWav(); };
    for (auto* b : { &generateButton, &prevSeed, &nextSeed, &playButton, &saveButton })
        addAndMakeVisible (b);

    seedLabel.setJustificationType (juce::Justification::centred);
    seedLabel.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (seedLabel);

    addAndMakeVisible (lane);
    addAndMakeVisible (dragChip);

    dragHint.setText ("Locked to FL's tempo. MIDI keys: C1 = chop 1, C#1 = chop 2 ...", juce::dontSendNotification);
    dragHint.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (dragHint);

    setResizable (true, true);
    setResizeLimits (780, 560, 1600, 1100);
    setSize (940, 640);

    refreshState();
    startTimerHz (30);
}

BasementEditor::~BasementEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BasementEditor::setupCombo (Combo& c, const juce::String& name, const juce::String& paramId)
{
    c.label.setText (name, juce::dontSendNotification);
    c.label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    c.label.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (c.label);

    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId)))
        c.box.addItemList (param->choices, 1);
    addAndMakeVisible (c.box);
    c.attachment = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, paramId, c.box);
}

void BasementEditor::setupKnob (Knob& k, const juce::String& name, const juce::String& paramId)
{
    k.label.setText (name, juce::dontSendNotification);
    k.label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    k.label.setColour (juce::Label::textColourId, theme::textDim);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramId, k.slider);
}

//==============================================================================
void BasementEditor::refreshState()
{
    auto state = proc.getState();
    waveform.setState (state);
    lane.setState (state);
    shownVersion = proc.getStateVersion();
    seedLabel.setText ("seed " + juce::String (proc.getSeed()), juce::dontSendNotification);

    const int styleIdx = juce::roundToInt (proc.apvts.getRawParameterValue ("style")->load());
    description.setText (basement::getChopStyle ((basement::ChopStyle) styleIdx).description, juce::dontSendNotification);

    if (state != nullptr && state->sample != nullptr)
        sampleInfo.setText (state->sample->name + "   |   " + juce::String (state->sample->getSeconds(), 1) + "s, "
                                + juce::String (state->slices.sampleBars) + " bar ~" + juce::String (juce::roundToInt (state->slices.sampleBpm))
                                + " bpm, " + juce::String (state->slices.numSlices()) + " chops",
                            juce::dontSendNotification);
    else
        sampleInfo.setText ("No sample loaded", juce::dontSendNotification);

    dragChip.repaint();
}

void BasementEditor::timerCallback()
{
    if (proc.getStateVersion() != shownVersion)
        refreshState();

    const double beat = proc.getPlayheadBeat();
    lane.setPlayhead (beat);
    waveform.setActiveSlice (beat >= 0.0 ? proc.getLastSlice() : -1);

    const bool internal = proc.isInternalPlaying();
    playButton.setButtonText (internal ? "STOP" : "PLAY");
    playButton.setToggleState (internal, juce::dontSendNotification);
}

void BasementEditor::loadFile (const juce::File& f)
{
    juce::String error;
    if (! proc.loadSample (f, error))
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Basement", error);
}

bool BasementEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;mp3;aif;aiff;flac;ogg;m4a"))
            return true;
    return false;
}

void BasementEditor::filesDropped (const juce::StringArray& files, int, int)
{
    waveform.setDropHighlight (false);
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;mp3;aif;aiff;flac;ogg;m4a"))
        {
            loadFile (juce::File (f));
            return;
        }
}

void BasementEditor::chooseSample()
{
    chooser = std::make_unique<juce::FileChooser> ("Load sample", juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   "*.wav;*.mp3;*.aif;*.aiff;*.flac;*.ogg;*.m4a");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc) {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  loadFile (f);
                          });
}

void BasementEditor::saveWav()
{
    if (! proc.hasSample())
        return;

    const auto start = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile (proc.suggestedFileName());
    chooser = std::make_unique<juce::FileChooser> ("Save WAV", start, "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc) {
                              const auto f = fc.getResult();
                              if (f != juce::File())
                                  proc.renderToFile (f.withFileExtension ("wav"));
                          });
}

//==============================================================================
void BasementEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    g.setColour (theme::grid);
    g.drawHorizontalLine (56, 16.0f, (float) getWidth() - 16.0f);
}

void BasementEditor::resized()
{
    auto area = getLocalBounds().reduced (16, 8);

    auto header = area.removeFromTop (48);
    title.setBounds (header.removeFromLeft (150));
    subtitle.setBounds (header.removeFromLeft (140));
    description.setBounds (header);
    area.removeFromTop (12);

    auto info = area.removeFromTop (28);
    loadButton.setBounds (info.removeFromRight (130));
    sampleInfo.setBounds (info);
    area.removeFromTop (6);
    waveform.setBounds (area.removeFromTop (juce::jmax (110, area.getHeight() / 4)));
    area.removeFromTop (10);

    auto row = area.removeFromTop (46);
    const int comboW = (row.getWidth() - 3 * 10) / 4;
    for (auto* c : { &style, &bars, &chop, &sampleBars })
    {
        auto cell = row.removeFromLeft (comboW);
        c->label.setBounds (cell.removeFromTop (16));
        c->box.setBounds (cell.removeFromTop (28));
        row.removeFromLeft (10);
    }
    area.removeFromTop (8);

    row = area.removeFromTop (96);
    const int knobW = row.getWidth() / 6;
    for (auto* k : { &pitch, &density, &chaos, &tone, &tempo, &volume })
    {
        auto cell = row.removeFromLeft (knobW);
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell.reduced (8, 0));
    }
    area.removeFromTop (8);

    row = area.removeFromTop (34);
    generateButton.setBounds (row.removeFromLeft (150));
    row.removeFromLeft (10);
    prevSeed.setBounds (row.removeFromLeft (34));
    seedLabel.setBounds (row.removeFromLeft (110));
    nextSeed.setBounds (row.removeFromLeft (34));
    row.removeFromLeft (10);
    playButton.setBounds (row.removeFromLeft (90));
    saveButton.setBounds (row.removeFromRight (110));
    area.removeFromTop (8);

    auto bottom = area.removeFromBottom (36);
    dragChip.setBounds (bottom.removeFromRight (180));
    dragHint.setBounds (bottom);
    area.removeFromBottom (8);

    lane.setBounds (area);
}

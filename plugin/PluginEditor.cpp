#include "PluginEditor.h"
#include "MidiWriter.h"

//==============================================================================
DragChip::DragChip (BasementProcessor& p, juce::String name, int layerMask, juce::Colour c)
    : processor (p), label (std::move (name)), layers (layerMask), colour (c)
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void DragChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (colour.withAlpha (isMouseOver() ? 0.35f : 0.18f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (colour);
    g.drawRoundedRectangle (r, 5.0f, 1.5f);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (label, r, juce::Justification::centred);
}

void DragChip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4)
        return;

    dragging = true;
    const auto file = processor.writeMidiToTempFile (layers);
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                [safe = juce::Component::SafePointer<DragChip> (this)] {
                                                                    if (safe != nullptr)
                                                                        safe->dragging = false;
                                                                });
}

//==============================================================================
BasementEditor::BasementEditor (BasementProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      dragAll (p, "DRAG ALL", basement::LayerAll, theme::text),
      dragMelody (p, "MELODY", basement::LayerMelody, theme::accent),
      dragChords (p, "CHORDS", basement::LayerChords, theme::chords),
      dragBass (p, "808", basement::LayerBass, theme::bass)
{
    setLookAndFeel (&lnf);

    title.setText ("BASEMENT", juce::dontSendNotification);
    title.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, theme::accent);
    addAndMakeVisible (title);

    subtitle.setText ("underground melody generator", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (subtitle);

    description.setColour (juce::Label::textColourId, theme::textDim);
    description.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (description);

    setupCombo (style, "STYLE", "style");
    setupCombo (key, "KEY", "key");
    setupCombo (scale, "SCALE", "scale");
    setupCombo (bars, "BARS", "bars");
    setupCombo (octave, "OCTAVE", "octave");

    setupKnob (density, "DENSITY", "density");
    setupKnob (darkness, "DARKNESS", "darkness");
    setupKnob (swing, "SWING", "swing");
    setupKnob (humanize, "HUMANIZE", "humanize");
    setupKnob (tempo, "TEMPO", "tempo");
    setupKnob (volume, "VOLUME", "volume");

    setupToggle (melodyToggle, "Melody", "melody");
    setupToggle (chordsToggle, "Chords", "chords");
    setupToggle (bassToggle, "808", "bass");
    setupToggle (soundToggle, "Sound", "sound");

    generateButton.setColour (juce::TextButton::buttonColourId, theme::accent);
    generateButton.onClick = [this] { proc.randomiseSeed(); };
    prevSeed.onClick = [this] { proc.nudgeSeed (-1); };
    nextSeed.onClick = [this] { proc.nudgeSeed (1); };
    playButton.setClickingTogglesState (false);
    playButton.onClick = [this] { proc.setInternalPlaying (! proc.isInternalPlaying()); };
    saveButton.onClick = [this] { saveMidi(); };

    for (auto* b : { &generateButton, &prevSeed, &nextSeed, &playButton, &saveButton })
        addAndMakeVisible (b);

    seedLabel.setJustificationType (juce::Justification::centred);
    seedLabel.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (seedLabel);

    addAndMakeVisible (roll);
    for (auto* c : { &dragAll, &dragMelody, &dragChords, &dragBass })
        addAndMakeVisible (c);

    dragHint.setText ("drag into your DAW  ->", juce::dontSendNotification);
    dragHint.setColour (juce::Label::textColourId, theme::textDim);
    dragHint.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (dragHint);

    setResizable (true, true);
    setResizeLimits (760, 520, 1600, 1000);
    setSize (920, 600);

    refreshPattern();
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

void BasementEditor::setupToggle (Toggle& t, const juce::String& name, const juce::String& paramId)
{
    t.button.setButtonText (name);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, paramId, t.button);
}

//==============================================================================
void BasementEditor::refreshPattern()
{
    roll.setPattern (proc.getPattern());
    shownVersion = proc.getPatternVersion();
    seedLabel.setText ("seed " + juce::String (proc.getSeed()), juce::dontSendNotification);

    const auto s = proc.currentSettings();
    description.setText (basement::getStyle (s.style).description, juce::dontSendNotification);
}

void BasementEditor::timerCallback()
{
    if (proc.getPatternVersion() != shownVersion)
        refreshPattern();

    roll.setPlayhead (proc.getPlayheadBeat());

    const bool internal = proc.isInternalPlaying();
    playButton.setButtonText (internal ? "STOP" : "PLAY");
    playButton.setToggleState (internal, juce::dontSendNotification);
}

void BasementEditor::saveMidi()
{
    const auto start = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile (proc.suggestedFileName (basement::LayerAll));
    chooser = std::make_unique<juce::FileChooser> ("Save MIDI", start, "*.mid");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc) {
                              auto file = fc.getResult();
                              if (file == juce::File())
                                  return;
                              const auto data = proc.midiData (basement::LayerAll);
                              file.withFileExtension ("mid").replaceWithData (data.getData(), data.getSize());
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
    subtitle.setBounds (header.removeFromLeft (220));
    description.setBounds (header);
    area.removeFromTop (12);

    // Row 1: choices
    auto row = area.removeFromTop (46);
    const int comboW = (row.getWidth() - 4 * 10) / 5;
    for (auto* c : { &style, &key, &scale, &bars, &octave })
    {
        auto cell = row.removeFromLeft (comboW);
        c->label.setBounds (cell.removeFromTop (16));
        c->box.setBounds (cell.removeFromTop (28));
        row.removeFromLeft (10);
    }
    area.removeFromTop (10);

    // Row 2: knobs + toggles
    row = area.removeFromTop (100);
    auto toggles = row.removeFromRight (120);
    const int knobW = row.getWidth() / 6;
    for (auto* k : { &density, &darkness, &swing, &humanize, &tempo, &volume })
    {
        auto cell = row.removeFromLeft (knobW);
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell.reduced (6, 0));
    }
    const int toggleH = toggles.getHeight() / 4;
    for (auto* t : { &melodyToggle, &chordsToggle, &bassToggle, &soundToggle })
        t->button.setBounds (toggles.removeFromTop (toggleH));
    area.removeFromTop (10);

    // Row 3: transport + seed
    row = area.removeFromTop (34);
    generateButton.setBounds (row.removeFromLeft (150));
    row.removeFromLeft (10);
    prevSeed.setBounds (row.removeFromLeft (34));
    seedLabel.setBounds (row.removeFromLeft (110));
    nextSeed.setBounds (row.removeFromLeft (34));
    row.removeFromLeft (10);
    playButton.setBounds (row.removeFromLeft (90));
    saveButton.setBounds (row.removeFromRight (110));
    area.removeFromTop (10);

    // Bottom: drag chips
    auto bottom = area.removeFromBottom (36);
    for (auto* c : { &dragBass, &dragChords, &dragMelody, &dragAll })
    {
        c->setBounds (bottom.removeFromRight (c == &dragAll ? 120 : 96));
        bottom.removeFromRight (8);
    }
    dragHint.setBounds (bottom);
    area.removeFromBottom (10);

    roll.setBounds (area);
}

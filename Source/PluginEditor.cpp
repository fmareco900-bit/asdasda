#include "PluginEditor.h"

//==============================================================================
// Motivos japoneses dibujados con juce::Path
namespace
{
constexpr float kTwoPi = 6.28318530718f;

// Petalo de sakura (con la pequena muesca en la punta), longitud 1, apunta hacia arriba
juce::Path makePetal()
{
    juce::Path p;
    p.startNewSubPath (0.0f, 0.0f);
    p.cubicTo (-0.55f, -0.25f, -0.70f, -0.95f, -0.16f, -1.00f);
    p.lineTo (0.0f, -0.88f);
    p.lineTo (0.16f, -1.00f);
    p.cubicTo (0.70f, -0.95f, 0.55f, -0.25f, 0.0f, 0.0f);
    p.closeSubPath();
    return p;
}

// Flor de sakura minimalista: 5 petalos + estambres
void drawBlossom (juce::Graphics& g, juce::Point<float> c, float r, float rot, float alpha = 1.0f)
{
    static const juce::Path petal = makePetal();

    juce::Path flower;
    for (int k = 0; k < 5; ++k)
    {
        juce::Path p (petal);
        p.applyTransform (juce::AffineTransform::scale (r).rotated (rot + (float) k * kTwoPi / 5.0f).translated (c.x, c.y));
        flower.addPath (p);
    }

    g.setColour (ui::crimson.withAlpha (0.10f * alpha));
    g.fillPath (flower);
    g.setColour (ui::crimson.withAlpha (0.85f * alpha));
    g.strokePath (flower, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (ui::ink.withAlpha (0.70f * alpha));
    for (int k = 0; k < 5; ++k)
    {
        const float a = rot + ((float) k + 0.5f) * kTwoPi / 5.0f;
        const float ex = c.x + std::sin (a) * r * 0.48f, ey = c.y - std::cos (a) * r * 0.48f;
        g.drawLine (c.x, c.y, ex, ey, 0.7f);
        g.fillEllipse (ex - 1.1f, ey - 1.1f, 2.2f, 2.2f);
    }
}

// Rama de cerezo en un rincon; `t` la coloca/refleja
void drawBranch (juce::Graphics& g, const juce::AffineTransform& t)
{
    juce::Path branch;
    branch.startNewSubPath (0.0f, 12.0f);
    branch.cubicTo (40.0f, 0.0f, 85.0f, 38.0f, 150.0f, 24.0f);
    branch.startNewSubPath (58.0f, 15.0f);
    branch.cubicTo (66.0f, 34.0f, 78.0f, 50.0f, 98.0f, 62.0f);
    branch.startNewSubPath (106.0f, 30.0f);
    branch.cubicTo (118.0f, 16.0f, 134.0f, 8.0f, 158.0f, 6.0f);

    g.setColour (ui::ink.withAlpha (0.55f));
    g.strokePath (branch, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), t);

    struct B { float x, y, r, rot; };
    const B blossoms[] = { { 150.f, 24.f, 11.f, 0.3f }, { 98.f, 62.f, 9.f, 1.0f }, { 158.f, 6.f, 8.f, 2.0f },
                           { 60.f, 19.f, 7.f, 0.7f },   { 124.f, 33.f, 6.f, 1.5f } };
    const float s = t.getScaleFactor();
    for (auto& b : blossoms)
        drawBlossom (g, juce::Point<float> (b.x, b.y).transformedBy (t), b.r * s, b.rot);
}

// Patron seigaiha (olas concentricas), muy sutil
void drawSeigaiha (juce::Graphics& g, juce::Rectangle<int> area, float r)
{
    g.saveState();
    g.reduceClipRegion (area);

    int row = 0;
    for (float cy = (float) area.getY() + r; cy < (float) area.getBottom() + r; cy += r * 0.5f, ++row)
    {
        const float xOff = (row % 2 == 0) ? 0.0f : r;
        for (float cx = (float) area.getX() - r + xOff; cx < (float) area.getRight() + r; cx += 2.0f * r)
        {
            g.setColour (ui::bg);
            g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
            g.setColour (ui::crimson.withAlpha (0.30f));
            for (float k : { 1.0f, 0.72f, 0.44f })
                g.drawEllipse (cx - r * k, cy - r * k, 2.0f * r * k, 2.0f * r * k, 0.8f);
        }
    }
    g.restoreState();
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, const juce::String& subtitle)
{
    g.setColour (ui::panelBg);
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (ui::hairline);
    g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);

    // marca carmesi + titulo
    g.setColour (ui::crimson);
    g.drawLine (r.getX() + 16.0f, r.getY() + 19.0f, r.getX() + 28.0f, r.getY() + 19.0f, 1.4f);
    g.setFont (ui::makeFont (11.5f, true, 0.28f));
    g.drawText (title, juce::Rectangle<int> ((int) r.getX() + 34, (int) r.getY() + 11, 160, 16), juce::Justification::centredLeft);

    g.setColour (ui::inkSoft);
    g.setFont (ui::makeFont (9.5f, false, 0.22f));
    g.drawText (subtitle, juce::Rectangle<int> ((int) r.getRight() - 210, (int) r.getY() + 11, 168, 16), juce::Justification::centredRight);

    drawBlossom (g, { r.getRight() - 22.0f, r.getY() + 19.0f }, 7.5f, 0.4f, 0.85f);
}
} // namespace

//==============================================================================
Chain7stAudioProcessorEditor::Chain7stAudioProcessorEditor (Chain7stAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    setupChoice (keyChoice,   "pitchKey",   "KEY");
    setupChoice (scaleChoice, "pitchScale", "SCALE");

    setupKnob (speed,   "pitchSpeed",  "SPEED");
    setupKnob (amount,  "pitchAmount", "AMOUNT");

    setupKnob (hpf,     "hpf",    "HPF");
    setupKnob (eqLow,   "eqLow",  "LOW");
    setupKnob (eqMid,   "eqMid",  "MID");
    setupKnob (eqHigh,  "eqHigh", "HIGH");

    setupKnob (deEss,   "deEss",       "DE-ESS");
    setupKnob (thr,     "compThresh",  "THRESH");
    setupKnob (ratio,   "compRatio",   "RATIO");
    setupKnob (attack,  "compAttack",  "ATTACK");
    setupKnob (release, "compRelease", "RELEASE");
    setupKnob (makeup,  "compMakeup",  "MAKEUP");

    setupKnob (rvSize,  "rvSize", "ROOM");
    setupKnob (rvMix,   "rvMix",  "REV MIX");
    setupKnob (dlTime,  "dlTime", "DLY TIME");
    setupKnob (dlFb,    "dlFb",   "FEEDBACK");
    setupKnob (dlMix,   "dlMix",  "DLY MIX");

    // Selector de presets
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    presetCaption.setText ("PRESET", juce::dontSendNotification);
    presetCaption.setFont (ui::makeFont (10.0f, true, 0.2f));
    presetCaption.setColour (juce::Label::textColourId, ui::inkSoft);
    presetCaption.setJustificationType (juce::Justification::centredLeft);
    presetCaption.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (presetCaption);

    setSize (900, 580);
}

Chain7stAudioProcessorEditor::~Chain7stAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void Chain7stAudioProcessorEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& captionText)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<SliderAttachment> (proc.apvts, paramId, k.slider);

    k.caption.setText (captionText, juce::dontSendNotification);
    k.caption.setFont (ui::makeFont (10.5f, true, 0.14f));
    k.caption.setColour (juce::Label::textColourId, ui::ink.withAlpha (0.78f));
    k.caption.setJustificationType (juce::Justification::centred);
    k.caption.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.caption);
}

void Chain7stAudioProcessorEditor::setupChoice (Choice& c, const juce::String& paramId, const juce::String& captionText)
{
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId)))
        c.box.addItemList (param->choices, 1);
    addAndMakeVisible (c.box);
    c.attachment = std::make_unique<ComboBoxAttachment> (proc.apvts, paramId, c.box);

    c.caption.setText (captionText, juce::dontSendNotification);
    c.caption.setFont (ui::makeFont (10.5f, true, 0.14f));
    c.caption.setColour (juce::Label::textColourId, ui::ink.withAlpha (0.78f));
    c.caption.setJustificationType (juce::Justification::centredLeft);
    c.caption.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (c.caption);
}

void Chain7stAudioProcessorEditor::placeKnob (Knob& k, juce::Rectangle<int> cell)
{
    const int knobW = juce::jmin (70, cell.getWidth()), knobH = 88, capH = 14;
    const int y = cell.getY() + (cell.getHeight() - (capH + knobH)) / 2;
    k.caption.setBounds (cell.getX(), y, cell.getWidth(), capH);
    k.slider.setBounds (cell.getCentreX() - knobW / 2, y + capH, knobW, knobH);
}

//==============================================================================
void Chain7stAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace ui;
    g.fillAll (bg);

    const float w = (float) getWidth(), h = (float) getHeight();

    // Linea divisoria tipo trazo de pincel (sumi-e), fina en los extremos
    {
        juce::Path p;
        const float y = 87.0f, x0 = 28.0f, x1 = w - 28.0f, xm = w * 0.5f;
        p.startNewSubPath (x0, y);
        p.quadraticTo (xm, y - 1.8f, x1, y);
        p.quadraticTo (xm, y + 1.8f, x0, y);
        p.closeSubPath();
        g.setColour (ink.withAlpha (0.16f));
        g.fillPath (p);
    }

    // Ramas de cerezo en las esquinas derechas
    drawBranch (g, juce::AffineTransform::scale (-0.85f, 0.85f).translated (w + 8.0f, 4.0f));
    drawBranch (g, juce::AffineTransform::scale (-0.80f, -0.80f).translated (w + 6.0f, h + 4.0f));

    // Pie: olas seigaiha + flujo de senal
    drawSeigaiha (g, { 28, 536, 392, 44 }, 13.0f);
    g.setColour (inkSoft);
    g.setFont (makeFont (9.5f, false, 0.25f));
    g.drawText ("PITCH  >  EQ  >  DE-ESS  >  COMP  >  SPACE", juce::Rectangle<int> (440, 544, 300, 16), juce::Justification::centred);

    // Paneles
    drawPanel (g, pitchPanel.toFloat(), "PITCH",    "AUTO-TUNE");
    drawPanel (g, eqPanel.toFloat(),    "EQ",       "HPF  /  3-BAND");
    drawPanel (g, dynPanel.toFloat(),   "DYNAMICS", "DE-ESSER  /  COMPRESSOR");
    drawPanel (g, spacePanel.toFloat(), "SPACE",    "REVERB  /  DELAY");

    // Logotipo: hinomaru + "Chain7st"
    g.setColour (crimson);
    g.fillEllipse (28.0f, 31.0f, 14.0f, 14.0f);

    juce::AttributedString logo;
    logo.append ("Chain", makeFont (38.0f, false, 0.01f), ink);
    logo.append ("7",     makeFont (38.0f, true),         crimson);
    logo.append ("st",    makeFont (38.0f, false, 0.01f), ink);
    logo.draw (g, juce::Rectangle<float> (52.0f, 12.0f, 300.0f, 46.0f));

    g.setColour (inkSoft);
    g.setFont (makeFont (10.5f, false, 0.35f));
    g.drawText ("ALL-IN-ONE VOCAL STRIP", juce::Rectangle<int> (54, 60, 260, 14), juce::Justification::centredLeft);
}

void Chain7stAudioProcessorEditor::resized()
{
    const int margin = 24, headerH = 86, footerH = 54, gap = 14, panelH = 208;

    presetCaption.setBounds (470, 24, 220, 14);
    presetBox.setBounds (470, 40, 220, 30);

    auto content = getLocalBounds().withTrimmedTop (headerH + 10).withTrimmedBottom (footerH).reduced (margin, 0);
    auto row1 = content.removeFromTop (panelH);
    content.removeFromTop (gap);
    auto row2 = content.removeFromTop (panelH);

    const int totalW1 = row1.getWidth() - gap;
    pitchPanel = row1.removeFromLeft (juce::roundToInt ((float) totalW1 * 3.5f / 7.5f));
    row1.removeFromLeft (gap);
    eqPanel = row1;

    const int totalW2 = row2.getWidth() - gap;
    dynPanel = row2.removeFromLeft (juce::roundToInt ((float) totalW2 * 6.0f / 11.0f));
    row2.removeFromLeft (gap);
    spacePanel = row2;

    auto inner = [] (juce::Rectangle<int> p) { return p.reduced (16, 0).withTrimmedTop (34).withTrimmedBottom (10); };

    // PITCH: columna de selectores + 2 perillas
    {
        auto a = inner (pitchPanel);
        auto col = a.removeFromLeft (juce::roundToInt ((float) a.getWidth() * 1.5f / 3.5f)).reduced (6, 0);
        const int groupH = 14 + 4 + 28, totalH = groupH * 2 + 18;
        int y = col.getY() + (col.getHeight() - totalH) / 2;

        keyChoice.caption.setBounds (col.getX(), y, col.getWidth(), 14);
        keyChoice.box.setBounds (col.getX(), y + 18, col.getWidth(), 28);
        y += groupH + 18;
        scaleChoice.caption.setBounds (col.getX(), y, col.getWidth(), 14);
        scaleChoice.box.setBounds (col.getX(), y + 18, col.getWidth(), 28);

        const int cw = a.getWidth() / 2;
        placeKnob (speed,  a.removeFromLeft (cw));
        placeKnob (amount, a);
    }

    // EQ: 4 perillas
    {
        auto a = inner (eqPanel);
        const int cw = a.getWidth() / 4;
        placeKnob (hpf,    a.removeFromLeft (cw));
        placeKnob (eqLow,  a.removeFromLeft (cw));
        placeKnob (eqMid,  a.removeFromLeft (cw));
        placeKnob (eqHigh, a);
    }

    // DYNAMICS: de-esser + compresor
    {
        auto a = inner (dynPanel);
        const int cw = a.getWidth() / 6;
        placeKnob (deEss,   a.removeFromLeft (cw));
        placeKnob (thr,     a.removeFromLeft (cw));
        placeKnob (ratio,   a.removeFromLeft (cw));
        placeKnob (attack,  a.removeFromLeft (cw));
        placeKnob (release, a.removeFromLeft (cw));
        placeKnob (makeup,  a);
    }

    // SPACE: reverb + delay
    {
        auto a = inner (spacePanel);
        const int cw = a.getWidth() / 5;
        placeKnob (rvSize, a.removeFromLeft (cw));
        placeKnob (rvMix,  a.removeFromLeft (cw));
        placeKnob (dlTime, a.removeFromLeft (cw));
        placeKnob (dlFb,   a.removeFromLeft (cw));
        placeKnob (dlMix,  a);
    }
}

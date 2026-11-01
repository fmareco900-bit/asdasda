#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
// Paleta y utilidades de estilo: blanco perla, carmesi y negro mate
namespace ui
{
    inline const juce::Colour bg          { 0xffFAFAFA };
    inline const juce::Colour panelBg     { 0xffFFFFFF };
    inline const juce::Colour crimson     { 0xffE60012 };
    inline const juce::Colour crimsonDeep { 0xffC91C1C };
    inline const juce::Colour ink         { 0xff1C1C1E };   // negro mate
    inline const juce::Colour inkSoft     { 0xff77777C };
    inline const juce::Colour hairline    { 0xffE9E4E4 };
    inline const juce::Colour track       { 0xffE7E3E3 };

    inline juce::Font makeFont (float height, bool bold = false, float kerning = 0.0f)
    {
        auto opts = juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (kerning);
        return juce::Font (opts);
    }
}

//==============================================================================
class Chain7stLookAndFeel : public juce::LookAndFeel_V4
{
public:
    Chain7stLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId,       ui::crimsonDeep);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId,  ui::crimson.withAlpha (0.25f));

        setColour (juce::ComboBox::backgroundColourId, ui::panelBg);
        setColour (juce::ComboBox::textColourId,       ui::ink);
        setColour (juce::ComboBox::outlineColourId,    ui::crimson);
        setColour (juce::ComboBox::arrowColourId,      ui::crimson);

        setColour (juce::PopupMenu::backgroundColourId,            ui::panelBg);
        setColour (juce::PopupMenu::textColourId,                  ui::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::crimson);
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

        setColour (juce::Label::textColourId, ui::ink);
    }

    //--------------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (5.0f);
        const float r      = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto  c      = b.getCentre();
        const float angle  = startAngle + sliderPos * (endAngle - startAngle);
        const float arcR   = r - 2.0f;

        // pista
        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (ui::track);
        g.strokePath (track, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // arco de valor (bipolar si el rango cruza el cero, p.ej. EQ)
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from = bipolar ? startAngle + (float) s.valueToProportionOfLength (0.0) * (endAngle - startAngle)
                                   : startAngle;
        juce::Path value;
        value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (ui::crimson);
        g.strokePath (value, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // cara de la perilla
        const float faceR = arcR - 7.0f;
        g.setColour (ui::panelBg);
        g.fillEllipse (c.x - faceR, c.y - faceR, faceR * 2.0f, faceR * 2.0f);
        g.setColour (juce::Colour (0xffDDD7D7));
        g.drawEllipse (c.x - faceR, c.y - faceR, faceR * 2.0f, faceR * 2.0f, 1.0f);

        // aguja fina
        const auto p1 = juce::Point<float> (c.x + std::sin (angle) * faceR * 0.38f, c.y - std::cos (angle) * faceR * 0.38f);
        const auto p2 = juce::Point<float> (c.x + std::sin (angle) * faceR * 0.86f, c.y - std::cos (angle) * faceR * 0.86f);
        juce::Path needle;
        needle.startNewSubPath (p1);
        needle.lineTo (p2);
        g.setColour (ui::ink);
        g.strokePath (needle, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // punto indicador sobre el arco
        const auto d = juce::Point<float> (c.x + std::sin (angle) * arcR, c.y - std::cos (angle) * arcR);
        g.setColour (ui::panelBg);
        g.fillEllipse (d.x - 4.0f, d.y - 4.0f, 8.0f, 8.0f);
        g.setColour (ui::crimson);
        g.drawEllipse (d.x - 4.0f, d.y - 4.0f, 8.0f, 8.0f, 1.6f);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (ui::makeFont (12.0f, true));
        l->setColour (juce::Label::textColourId, ui::crimsonDeep);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::textWhenEditingColourId, ui::ink);
        l->setColour (juce::Label::backgroundWhenEditingColourId, ui::panelBg);
        l->setColour (juce::Label::outlineWhenEditingColourId, ui::crimson);
        return l;
    }

    //--------------------------------------------------------------------------
    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        g.setColour (ui::panelBg);
        g.fillRoundedRectangle (r, 9.0f);
        g.setColour (ui::crimson.withAlpha (box.isPopupActive() ? 1.0f : 0.75f));
        g.drawRoundedRectangle (r, 9.0f, 1.2f);

        const float cx = (float) width - 18.0f, cy = (float) height * 0.5f;
        juce::Path arrow;
        arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
        arrow.lineTo (cx, cy + 2.0f);
        arrow.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (ui::crimson);
        g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui::makeFont (13.0f, true); }
    juce::Font getPopupMenuFont() override               { return ui::makeFont (13.0f, false); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (12, 1, box.getWidth() - 36, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
    }
};

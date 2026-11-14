#include "PluginEditor.h"

using namespace juce;

namespace
{
const Colour bgCol (0xff0e1013), panelCol (0xff161a1f), lineCol (0xff2a313b), fgCol (0xffe9e5dc), dimCol (0xff8e98a6), accCol (0xffff8a4c);
const Colour oscCols[4] = { Colour (0xff4cc9f0), Colour (0xfff472b6), Colour (0xfffacc15), Colour (0xff4ade80) };

void mixGains (float x, float y, float* g)
{
    g[0] = (1.f - x) * (1.f - y);
    g[1] = x * (1.f - y);
    g[2] = (1.f - x) * y;
    g[3] = x * y;
}
} // namespace

//==============================================================================
void VectorPad::setNorm (const char* id, float v)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (jlimit (0.f, 1.f, v)));
        prm->endChangeGesture();
    }
}

void VectorPad::move (const MouseEvent& e)
{
    const auto f = field();
    const float x = (e.position.x - f.getX()) / f.getWidth();
    const float y = (e.position.y - f.getY()) / f.getHeight();
    if (proc.apvts.getRawParameterValue ("venv")->load() > 0.5f)
    {
        setNorm (("vx" + String (selected)).toRawUTF8(), x);
        setNorm (("vy" + String (selected)).toRawUTF8(), y);
    }
    else
    {
        setNorm ("vx", x);
        setNorm ("vy", y);
    }
}

void VectorPad::paint (Graphics& g)
{
    const auto f = field();
    auto X = [&] (float x) { return f.getX() + x * f.getWidth(); };
    auto Y = [&] (float y) { return f.getY() + y * f.getHeight(); };
    auto val = [&] (const String& id) { return proc.apvts.getRawParameterValue (id)->load(); };
    const bool venv = val ("venv") > 0.5f, loop = val ("vloop") > 0.5f;

    g.setColour (Colour (0xff1c2128));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.f);

    const float lx = venv ? proc.engine.getLiveX() : val ("vx");
    const float ly = venv ? proc.engine.getLiveY() : val ("vy");
    float gains[4];
    mixGains (lx, ly, gains);
    const float cx[4] = { 0, 1, 0, 1 }, cy[4] = { 0, 0, 1, 1 };
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (getLocalBounds());
        for (int i = 0; i < 4; ++i)
        {
            const float rad = f.getWidth() * (0.12f + 0.55f * gains[i]);
            ColourGradient cg (oscCols[i].withAlpha (0.65f), X (cx[i]), Y (cy[i]), oscCols[i].withAlpha (0.f), X (cx[i]) + rad, Y (cy[i]), true);
            g.setGradientFill (cg);
            g.fillRect (getLocalBounds());
        }
    }

    g.setColour (lineCol);
    for (int q = 0; q <= 4; ++q)
    {
        g.drawLine (X (q / 4.f), f.getY(), X (q / 4.f), f.getBottom());
        g.drawLine (f.getX(), Y (q / 4.f), f.getRight(), Y (q / 4.f));
    }
    g.drawRect (f);

    g.setFont (Font (FontOptions (15.f, Font::bold)));
    const char* names[4] = { "A", "B", "C", "D" };
    for (int i = 0; i < 4; ++i)
    {
        g.setColour (oscCols[i]);
        g.drawText (names[i], Rectangle<float> (X (cx[i]) - 12.f, Y (cy[i]) + (cy[i] > 0.5f ? 2.f : -22.f), 24.f, 20.f), Justification::centred);
    }

    if (venv)
    {
        Path path;
        for (int k = 0; k < 4; ++k)
        {
            const float px = X (val ("vx" + String (k))), py = Y (val ("vy" + String (k)));
            if (k == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        if (loop)
            path.closeSubPath();
        g.setColour (dimCol);
        const float dashes[2] = { 4.f, 4.f };
        Path dashed;
        PathStrokeType (1.f).createDashedStroke (dashed, path, dashes, 2);
        g.fillPath (dashed);

        g.setFont (Font (FontOptions (12.f)));
        for (int k = 0; k < 4; ++k)
        {
            const float px = X (val ("vx" + String (k))), py = Y (val ("vy" + String (k)));
            const bool sel = (k == selected);
            g.setColour (sel ? accCol : Colour (0xff20262e));
            g.fillEllipse (px - 9.f, py - 9.f, 18.f, 18.f);
            g.setColour (sel ? accCol : dimCol);
            g.drawEllipse (px - 9.f, py - 9.f, 18.f, 18.f, 1.f);
            g.setColour (sel ? Colour (0xff1a0e06) : fgCol);
            g.drawText (String (k + 1), Rectangle<float> (px - 9.f, py - 9.f, 18.f, 18.f), Justification::centred);
        }
        g.setColour (fgCol);
        g.fillEllipse (X (lx) - 5.f, Y (ly) - 5.f, 10.f, 10.f);
    }
    else
    {
        g.setColour (accCol);
        g.drawEllipse (X (lx) - 13.f, Y (ly) - 13.f, 26.f, 26.f, 2.f);
        g.fillEllipse (X (lx) - 4.f, Y (ly) - 4.f, 8.f, 8.f);
    }
}

//==============================================================================
ProphorcelEditor::Ctl& ProphorcelEditor::addCtl (Section& s, const char* id, const char* label, const char* suffix)
{
    auto c = std::make_unique<Ctl>();
    c->label.setText (label, dontSendNotification);
    c->label.setColour (Label::textColourId, dimCol);
    c->label.setFont (Font (FontOptions (12.f)));
    c->slider.setSliderStyle (Slider::LinearHorizontal);
    c->slider.setTextBoxStyle (Slider::TextBoxRight, false, 66, 20);
    c->slider.setColour (Slider::trackColourId, accCol);
    c->slider.setColour (Slider::thumbColourId, fgCol);
    c->slider.setColour (Slider::backgroundColourId, lineCol);
    c->slider.setColour (Slider::textBoxTextColourId, fgCol);
    c->slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    c->slider.setTextValueSuffix (suffix);
    c->slider.setNumDecimalPlacesToDisplay (2);
    c->att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, c->slider);
    addAndMakeVisible (c->label);
    addAndMakeVisible (c->slider);
    s.ctls.push_back (c.get());
    ctlStore.push_back (std::move (c));
    return *ctlStore.back();
}

ProphorcelEditor::Combo& ProphorcelEditor::addCombo (Section& s, const char* id, const char* label)
{
    auto c = std::make_unique<Combo>();
    c->label.setText (label, dontSendNotification);
    c->label.setColour (Label::textColourId, dimCol);
    c->label.setFont (Font (FontOptions (12.f)));
    if (auto* prm = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c->box.addItemList (prm->choices, 1);
    c->att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c->box);
    addAndMakeVisible (c->label);
    addAndMakeVisible (c->box);
    s.combos.push_back (c.get());
    comboStore.push_back (std::move (c));
    return *comboStore.back();
}

ProphorcelEditor::ProphorcelEditor (ProphorcelProcessor& p)
    : AudioProcessorEditor (&p), proc (p), pad (p),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (nullptr);
    sections.reserve (12);

    // Oscillateurs
    {
        sections.push_back ({ "Oscillateurs", {}, {}, {} });
        auto& s = sections.back();
        const char* tags[4] = { "A", "B", "C", "D" };
        for (int i = 0; i < 4; ++i)
        {
            const String n (i);
            addCombo (s, ("w" + n).toRawUTF8(), (String ("Onde ") + tags[i]).toRawUTF8());
            addCtl (s, ("s" + n).toRawUTF8(), (String ("Demi ") + tags[i]).toRawUTF8());
            addCtl (s, ("f" + n).toRawUTF8(), (String ("Fin ") + tags[i]).toRawUTF8(), " ct");
        }
    }
    { sections.push_back ({ "Filtre passe-bas", {}, {}, {} }); auto& s = sections.back();
      addCtl (s, "cut", "Coupure", " Hz"); addCtl (s, "res", "Résonance", " dB"); addCtl (s, "fltenv", "Enveloppe", " oct"); addCtl (s, "kt", "Suivi clav."); }
    { sections.push_back ({ "Enveloppe du filtre", {}, {}, {} }); auto& s = sections.back();
      addCtl (s, "fa", "Attaque", " s"); addCtl (s, "fd", "Déclin", " s"); addCtl (s, "fs", "Maintien"); addCtl (s, "fr", "Relâche", " s"); }
    { sections.push_back ({ "Enveloppe d'amplitude", {}, {}, {} }); auto& s = sections.back();
      addCtl (s, "aa", "Attaque", " s"); addCtl (s, "ad", "Déclin", " s"); addCtl (s, "as", "Maintien"); addCtl (s, "ar", "Relâche", " s"); }
    { sections.push_back ({ "LFO", {}, {}, {} }); auto& s = sections.back();
      addCombo (s, "lshape", "Forme"); addCtl (s, "lrate", "Vitesse", " Hz"); addCtl (s, "lpitch", "Hauteur", " ct"); addCtl (s, "lfilt", "Filtre", " ct"); }
    { sections.push_back ({ "Voix", {}, {}, {} }); auto& s = sections.back();
      addCombo (s, "mode", "Mode"); addCtl (s, "uni", "Unisson"); addCtl (s, "spread", "Écart", " ct"); addCtl (s, "glide", "Portamento", " s");
      addCtl (s, "bend", "Molette", " demi"); addCtl (s, "vela", "Vél. volume"); addCtl (s, "velf", "Vél. filtre", " oct"); }
    { sections.push_back ({ "Chorus / ensemble", {}, {}, {} }); auto& s = sections.back();
      addCtl (s, "chmix", "Mélange"); addCtl (s, "chrate", "Vitesse", " Hz"); addCtl (s, "chdepth", "Profondeur"); }
    { sections.push_back ({ "Écho", {}, {}, {} }); auto& s = sections.back();
      addCtl (s, "dt", "Durée", " s"); addCtl (s, "dfb", "Répétitions"); addCtl (s, "dmix", "Mélange"); }
    { sections.push_back ({ "Durées de l'enveloppe vectorielle", {}, {}, {} }); auto& s = sections.back();
      addCtl (s, "vt0", "1 → 2", " s"); addCtl (s, "vt1", "2 → 3", " s"); addCtl (s, "vt2", "3 → 4", " s"); addCtl (s, "vt3", "4 → 1", " s"); }

    // Pad et sélection des points
    addAndMakeVisible (pad);
    for (int i = 0; i < 4; ++i)
    {
        selBtn[i].setButtonText ("Point " + String (i + 1));
        selBtn[i].setClickingTogglesState (false);
        selBtn[i].onClick = [this, i]
        {
            pad.selected = i;
            for (int k = 0; k < 4; ++k)
                selBtn[k].setToggleState (k == i, dontSendNotification);
        };
        addAndMakeVisible (selBtn[i]);
    }
    selBtn[0].setToggleState (true, dontSendNotification);
    for (auto& b : selBtn)
        b.setColour (TextButton::buttonOnColourId, accCol);
    venvOnAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "venv", venvOn);
    venvLoopAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "vloop", venvLoop);
    addAndMakeVisible (venvOn);
    addAndMakeVisible (venvLoop);

    // Sons
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (24, 96);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (24.f);

    setResizable (false, false);
    setSize (1040, 790);
}

ProphorcelEditor::~ProphorcelEditor() = default;

void ProphorcelEditor::paint (Graphics& g)
{
    g.fillAll (bgCol);
    g.setColour (fgCol);
    g.setFont (Font (FontOptions (28.f, Font::bold)));
    g.drawText ("PROPH", Rectangle<int> (16, 10, 100, 36), Justification::centredLeft);
    g.setColour (accCol);
    g.drawText ("OR", Rectangle<int> (16 + 92, 10, 50, 36), Justification::centredLeft);
    g.setColour (fgCol);
    g.drawText ("CEL", Rectangle<int> (16 + 128, 10, 80, 36), Justification::centredLeft);
    g.setColour (dimCol);
    g.setFont (Font (FontOptions (12.f)));
    g.drawText ("synthétiseur vectoriel · 4 oscillateurs · 8 voix", Rectangle<int> (16, 44, 360, 16), Justification::centredLeft);

    for (auto& s : sections)
    {
        g.setColour (panelCol);
        g.fillRoundedRectangle (s.area.toFloat(), 8.f);
        g.setColour (lineCol);
        g.drawRoundedRectangle (s.area.toFloat().reduced (0.5f), 8.f, 1.f);
        g.setColour (dimCol);
        g.setFont (Font (FontOptions (11.f, Font::bold)));
        g.drawText (s.title.toUpperCase(), s.area.getX() + 12, s.area.getY() + 6, s.area.getWidth() - 24, 16, Justification::centredLeft);
    }
    g.setColour (panelCol);
    g.fillRoundedRectangle (padArea.toFloat(), 8.f);
    g.setColour (lineCol);
    g.drawRoundedRectangle (padArea.toFloat().reduced (0.5f), 8.f, 1.f);
    g.setColour (dimCol);
    g.setFont (Font (FontOptions (11.f, Font::bold)));
    g.drawText ("JOYSTICK VECTORIEL", padArea.getX() + 12, padArea.getY() + 6, padArea.getWidth() - 24, 16, Justification::centredLeft);
}

void ProphorcelEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    auto top = r.removeFromTop (48);
    presetBox.setBounds (top.removeFromRight (220).withTrimmedTop (8).withHeight (28));
    r.removeFromTop (6);

    auto kb = r.removeFromBottom (92);
    keyboard.setBounds (kb);
    r.removeFromBottom (10);

    // colonne de gauche : pad
    auto left = r.removeFromLeft (340);
    padArea = left;
    auto inner = left.reduced (10);
    inner.removeFromTop (20);
    pad.setBounds (inner.removeFromTop (inner.getWidth()));
    inner.removeFromTop (8);
    auto row = inner.removeFromTop (26);
    venvOn.setBounds (row.removeFromLeft (170));
    venvLoop.setBounds (row);
    inner.removeFromTop (4);
    auto brow = inner.removeFromTop (28);
    const int bw = brow.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        selBtn[i].setBounds (brow.removeFromLeft (bw).reduced (2, 0));
    inner.removeFromTop (6);
    // les durées de l'enveloppe vectorielle sont dans la dernière section : elle occupe le bas de la colonne
    {
        auto& s = sections.back();
        s.area = inner.withTrimmedTop (0).withHeight (juce::jmin (inner.getHeight(), 138)).withY (inner.getY());
    }

    r.removeFromLeft (12);

    // colonnes de droite : 2 colonnes de sections
    const int colW = (r.getWidth() - 12) / 2;
    auto colA = r.removeFromLeft (colW);
    r.removeFromLeft (12);
    auto colB = r;

    // répartition : A = Oscillateurs, Filtre, Env. filtre ; B = Env. amp, LFO, Voix, Chorus, Écho
    auto place = [&] (Rectangle<int>& col, int idx, int h)
    {
        auto& s = sections[(size_t) idx];
        s.area = col.removeFromTop (h);
        col.removeFromTop (8);
    };
    place (colA, 0, 262);
    place (colA, 1, 124);
    place (colA, 2, 124);
    place (colA, 6, 100);
    place (colB, 3, 124);
    place (colB, 4, 124);
    place (colB, 5, 202);
    place (colB, 7, 100);

    auto layoutSection = [&] (Section& s)
    {
        auto a = s.area.reduced (12, 8);
        a.removeFromTop (18);
        const int n = (int) (s.ctls.size() + s.combos.size());
        if (n == 0)
            return;
        const int rowH = juce::jmin (26, a.getHeight() / n);
        auto put = [&] (Label& l, Component& c)
        {
            auto line = a.removeFromTop (rowH);
            l.setBounds (line.removeFromLeft (74));
            c.setBounds (line);
        };
        if (s.title == "Oscillateurs")
        {
            // trois lignes par oscillateur : onde, demi-tons, accord fin
            for (size_t i = 0; i < 4; ++i)
            {
                auto line = a.removeFromTop (rowH);
                auto& cb = *s.combos[i];
                cb.label.setBounds (line.removeFromLeft (54));
                cb.box.setBounds (line.removeFromLeft (120).reduced (0, 2));
                line.removeFromLeft (8);
                auto& semi = *s.ctls[i * 2];
                auto& fine = *s.ctls[i * 2 + 1];
                const int w = line.getWidth() / 2;
                semi.label.setBounds (0, 0, 0, 0);
                fine.label.setBounds (0, 0, 0, 0);
                semi.slider.setBounds (line.removeFromLeft (w));
                fine.slider.setBounds (line);
                a.removeFromTop (juce::jmax (0, 10));
            }
            return;
        }
        for (auto* cb : s.combos)
        {
            auto line = a.removeFromTop (rowH);
            cb->label.setBounds (line.removeFromLeft (74));
            cb->box.setBounds (line.removeFromLeft (150).reduced (0, 2));
        }
        for (auto* c : s.ctls)
            put (c->label, c->slider);
    };
    for (auto& s : sections)
        layoutSection (s);
}

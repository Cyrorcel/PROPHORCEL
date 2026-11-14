#pragma once

#include "PluginProcessor.h"

// Joystick vectoriel : un point libre, ou les 4 points de l'enveloppe vectorielle.
class VectorPad : public juce::Component, private juce::Timer
{
public:
    explicit VectorPad (ProphorcelProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override { move (e); }
    void mouseDrag (const juce::MouseEvent& e) override { move (e); }

    int selected = 0;

private:
    void timerCallback() override { repaint(); }
    void move (const juce::MouseEvent& e);
    void setNorm (const char* id, float v);
    juce::Rectangle<float> field() const { return getLocalBounds().toFloat().reduced (22.f); }

    ProphorcelProcessor& proc;
};

class ProphorcelEditor : public juce::AudioProcessorEditor
{
public:
    explicit ProphorcelEditor (ProphorcelProcessor&);
    ~ProphorcelEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Ctl
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Combo
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    };
    struct Section
    {
        juce::String title;
        std::vector<Ctl*> ctls;
        std::vector<Combo*> combos;
        juce::Rectangle<int> area;
    };

    Ctl& addCtl (Section& s, const char* id, const char* label, const char* suffix = "");
    Combo& addCombo (Section& s, const char* id, const char* label);

    ProphorcelProcessor& proc;

    VectorPad pad;
    juce::ComboBox presetBox;
    juce::TextButton selBtn[4];
    juce::ToggleButton venvOn { "Enveloppe vectorielle" }, venvLoop { "Boucle" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> venvOnAtt, venvLoopAtt;
    juce::MidiKeyboardComponent keyboard;

    std::vector<std::unique_ptr<Ctl>> ctlStore;
    std::vector<std::unique_ptr<Combo>> comboStore;
    std::vector<Section> sections;
    juce::Rectangle<int> padArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProphorcelEditor)
};

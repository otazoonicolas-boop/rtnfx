#include "PluginEditor.h"
#include "Generated/param_table.h"
#include <cstring>

using namespace juce;

static const Colour kBg (0xff15171c), kPanel (0xff20232b), kAccent (0xfff2a33a), kText (0xffd8dbe2), kDim (0xff8b909c);

SR1Editor::SR1Editor (SR1Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    laf.setColour (Slider::rotarySliderFillColourId, kAccent);
    laf.setColour (Slider::rotarySliderOutlineColourId, Colour (0xff3a3f4b));
    laf.setColour (Slider::thumbColourId, kText);
    laf.setColour (Slider::textBoxTextColourId, kText);
    laf.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    laf.setColour (Label::textColourId, kDim);
    laf.setColour (ToggleButton::tickColourId, kAccent);
    laf.setColour (ToggleButton::textColourId, kText);
    laf.setColour (ComboBox::backgroundColourId, Colour (0xff2c303a));
    laf.setColour (ComboBox::textColourId, kText);
    laf.setColour (ComboBox::outlineColourId, Colour (0xff3a3f4b));
    laf.setColour (PopupMenu::backgroundColourId, kPanel);
    setLookAndFeel (&laf);

    panels.resize ((size_t) kNumGroups);
    for (int g = 0; g < kNumGroups; ++g) panels[(size_t) g].group = g;

    for (int i = 0; i < kNumParams; ++i)
    {
        const ParamInfo& info = kParams[i];
        auto c = std::make_unique<Cell>();
        c->paramIndex = i;
        auto* param = proc.apvts.getParameter (info.id);
        const bool isPower = info.kind == ParamInfo::Bool && String (info.id).endsWith ("On");
        Panel& panel = panels[(size_t) info.group];

        if (info.kind == ParamInfo::Float)
        {
            c->slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
            c->slider->setTextBoxStyle (Slider::TextBoxBelow, false, 70, 16);
            c->slider->setDoubleClickReturnValue (true, info.defV);
            addAndMakeVisible (*c->slider);
            c->sAtt = std::make_unique<SliderParameterAttachment> (*param, *c->slider, nullptr);
            c->label.setText (info.label, dontSendNotification);
            c->label.setJustificationType (Justification::centred);
            c->label.setFont (FontOptions (12.0f));
            addAndMakeVisible (c->label);
        }
        else if (info.kind == ParamInfo::Choice)
        {
            c->combo = std::make_unique<ComboBox>();
            c->combo->addItemList (StringArray::fromTokens (String (info.choices), "|", ""), 1);
            addAndMakeVisible (*c->combo);
            c->cAtt = std::make_unique<ComboBoxParameterAttachment> (*param, *c->combo, nullptr);
            c->label.setText (info.label, dontSendNotification);
            c->label.setJustificationType (Justification::centred);
            c->label.setFont (FontOptions (12.0f));
            addAndMakeVisible (c->label);
        }
        else
        {
            c->toggle = std::make_unique<ToggleButton> (isPower ? String ("ON") : String (info.label));
            addAndMakeVisible (*c->toggle);
            c->bAtt = std::make_unique<ButtonParameterAttachment> (*param, *c->toggle, nullptr);
        }

        const int idx = (int) cells.size();
        cells.push_back (std::move (c));
        if (isPower) panel.powerCell = idx; else panel.cells.push_back (idx);
    }

    setSize (940, 700);
}

SR1Editor::~SR1Editor() { setLookAndFeel (nullptr); }

void SR1Editor::paint (Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAccent);
    g.setFont (FontOptions (22.0f, Font::bold));
    g.drawText ("SR1 VOCAL CHAIN", 16, 6, 400, 30, Justification::centredLeft);

    for (auto& p : panels)
    {
        g.setColour (kPanel);
        g.fillRoundedRectangle (p.bounds.toFloat(), 8.0f);
        g.setColour (kText);
        g.setFont (FontOptions (13.0f, Font::bold));
        g.drawText (String (kGroupNames[p.group]).toUpperCase(), p.bounds.getX() + 10, p.bounds.getY() + 4, 150, 22, Justification::centredLeft);
    }
}

void SR1Editor::resized()
{
    // Tres filas de paneles, ancho proporcional al numero de columnas.
    const int rows[3][3] = { {0, 1, 2}, {3, 4, 5}, {6, 7, 8} };
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (34);
    const int rowH = area.getHeight() / 3;

    for (int r = 0; r < 3; ++r)
    {
        auto rowArea = area.removeFromTop (rowH);
        int totalCols = 0;
        int colsOf[3];
        for (int k = 0; k < 3; ++k)
        {
            colsOf[k] = ((int) panels[(size_t) rows[r][k]].cells.size() + 1) / 2;
            totalCols += colsOf[k];
        }
        const int gap = 8;
        const float unit = (float) (rowArea.getWidth() - gap * 2) / (float) totalCols;

        int x = rowArea.getX();
        for (int k = 0; k < 3; ++k)
        {
            Panel& p = panels[(size_t) rows[r][k]];
            const int w = (int) std::round (unit * (float) colsOf[k]);
            p.bounds = Rectangle<int> (x, rowArea.getY() + 2, w, rowH - 6);
            x += w + gap;

            if (p.powerCell >= 0)
                cells[(size_t) p.powerCell]->toggle->setBounds (p.bounds.getRight() - 62, p.bounds.getY() + 4, 56, 22);

            auto inner = p.bounds.withTrimmedTop (28).reduced (4, 2);
            const int cw = inner.getWidth() / colsOf[k];
            const int ch = inner.getHeight() / 2;
            for (size_t n = 0; n < p.cells.size(); ++n)
            {
                Cell& c = *cells[(size_t) p.cells[n]];
                Rectangle<int> cell (inner.getX() + (int) (n % (size_t) colsOf[k]) * cw,
                                     inner.getY() + (int) (n / (size_t) colsOf[k]) * ch, cw, ch);
                if (c.slider)
                {
                    c.label.setBounds (cell.removeFromTop (16));
                    c.slider->setBounds (cell);
                }
                else if (c.combo)
                {
                    c.label.setBounds (cell.removeFromTop (16));
                    c.combo->setBounds (cell.withSizeKeepingCentre (jmin (cw - 8, 110), 24));
                }
                else
                    c.toggle->setBounds (cell.withSizeKeepingCentre (jmin (cw - 4, 110), 26));
            }
        }
    }
}

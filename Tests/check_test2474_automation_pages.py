#!/usr/bin/env python3
"""Regression checks for TEST2.4.74 keyboard, REAPER API and accessibility.
Read Windows C++ sources explicitly as UTF-8.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
e = (root / "Source/PluginEditor.cpp").read_text(encoding="utf-8")
eh = (root / "Source/PluginEditor.h").read_text(encoding="utf-8")
p = (root / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")
ph = (root / "Source/PluginProcessor.h").read_text(encoding="utf-8")
workflow = (root / ".github/workflows/build-windows.yml").read_text(encoding="utf-8")
assert "bool LB303AudioProcessorEditor::ParameterGrid::keyPressed" in e
assert "return editor.handleKey (k, Control::grid) || juce::ComboBox::keyPressed (k);" in e
assert "return editor.handleKey (k, Control::value) || juce::Slider::keyPressed (k);" in e
assert "return editor.handleKey (k, Control::edit) || juce::TextEditor::keyPressed (k);" in e
assert "mods.isCtrlDown() && mods.isShiftDown() && !mods.isAltDown() && letter ('a')" in e
assert "changeContext(mods.isShiftDown() ? -1 : 1)" in e
assert "context = (context + (direction < 0 ? pages - 1 : 1)) % pages;" in e
assert "Ctrl+Shift+A" in (root / "README.md").read_text(encoding="utf-8")
assert "getVST3ClientExtensions() override" in ph
assert "GetFXEnvelope" in p and '"ACTIVE"' in p
assert "GetSetEnvelopeInfo_String" in p
assert "getReaperEnvelopeActive" in p and "toggleReaperEnvelopeActive" in p
assert "GetTouchedOrFocusedFX" in p and "TrackFX_GetParamName" in p
assert "getParameters().indexOf(param)" in p
assert 'if (!getParamName(owner, fxIdx, parameter, name, int(sizeof(name)))) return {};' in p
assert '"ARM", "0"' in p and '"VISIBLE", "0"' in p
assert "selectedAutomationParameter() const" in eh
assert "return processor.parameter(globalIndex());" in e
assert "getHostAutomationEnvelopeState" in e and "syncAutomationIndicator" in e
assert "observedAutomationState == 1" in e
assert "labelFor(selected)" in e
assert 'param->setValueNotifyingHost(param->getValue());' in e
assert "check_test2474_automation_pages.py" in workflow
assert workflow.count('TEST2.4.75-Windows-x64')==3
assert (root / "Help/LB303Help.html").read_text(encoding="utf-8").count("TEST2.4.74 — Automation and page navigation")==7
# In all four pages, an editable parameter should be a VST3 ID, not a transient FX display index.
assert "return visibleFxParameters.at" in e and "return basslineParameterIndex" in e
assert "sequencerParameterIndex" in e
# Avoid the Windows JUCE Ctrl == Command rejection seen in LR-608 TEST36.1.
assert "mods.isCtrlDown() && ! mods.isCommandDown() && mods.isShiftDown()" not in e
print("PASS: TEST2.4.74 automation keys, REAPER envelope API, NVDA and reverse pages")

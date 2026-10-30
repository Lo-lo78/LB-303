"""TEST2.4.42: guards for the Note/Length/Velocity toggles.

The JUCE-free SequencerState executable smoke test was run during development;
GitHub Actions separately compiles the full JUCE plugin after this regression.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
h = (root / "Source/SequencerState.h").read_text(encoding="utf-8")
s = (root / "Source/SequencerState.cpp").read_text(encoding="utf-8")
e = (root / "Source/SequencerStepEditor.cpp").read_text(encoding="utf-8")
eh = (root / "Source/SequencerStepEditor.h").read_text(encoding="utf-8")
workflow = (root / ".github/workflows/build-windows.yml").read_text(encoding="utf-8")

# No per-step Accent and no change to the original seven-page Alt+Q scheme.
assert '"Note", "Length", "Velocity", "Repeat", "Shift", "Launch Step", "Parameters"' in e
assert '"Accent"' not in e
assert "SequencerLayer::accent" not in s
assert 'int accent =' not in h
assert "else if (page == 6) openParameterPicker();" in e
assert "else if (page == 1) toggleCurrentLength();" in e
assert "void toggleCurrentLength();" in eh
assert "void SequencerStepEditor::toggleCurrentNote()" in e
assert "void SequencerStepEditor::toggleCurrentLength()" in e
assert "void SequencerStepEditor::toggleCurrentVelocity()" in e
assert "else if (page == 2) toggleCurrentVelocity();" in e
assert "change.before.velocityBeforeMax : 127;" in e
assert "void toggleCurrentVelocity();" in eh

# Enter must use per-step remembered state, not hardwired constants.
assert "change.before.note >= 128 ? change.before.noteBeforePause : 128" in e
assert "change.before.lengthBeforeLegato : 101;" in e
assert "change.before.note >= 128 ? 60 : 128" not in e
assert "after.length == 101 ? 100 : 101" not in e
assert "if (requestedNote < 128)" in s
assert "if (requestedLength != 101)" in s

# Memories survive atomic-state copies, snapshots, and project saves.
assert "int noteBeforePause = 60;" in h
assert "int lengthBeforeLegato = 50;" in h
assert "int velocityBeforeMax = 100;" in h
assert "std::atomic<int> noteBeforePause { 60 };" in h
assert "std::atomic<int> lengthBeforeLegato { 50 };" in h
assert "std::atomic<int> velocityBeforeMax { 100 };" in h
assert "result.noteBeforePause = loadRelaxed (step.noteBeforePause);" in s
assert "result.lengthBeforeLegato = loadRelaxed (step.lengthBeforeLegato);" in s
assert "result.velocityBeforeMax = loadRelaxed (step.velocityBeforeMax);" in s
assert "source.noteBeforePause" in s and "source.lengthBeforeLegato" in s
assert "destinationStep.noteBeforePause" in s and "destinationStep.lengthBeforeLegato" in s
assert "destinationStep.velocityBeforeMax" in s
assert "stream.writeInt (7);" in s
assert "formatVersion == 4 || formatVersion == 5" in s  # consume legacy abandoned Accent byte
assert "formatVersion >= 5" in s
assert "formatVersion >= 7" in s
assert "formatVersion > 7" in s
assert "if (requestedVelocity < 127)" in s
assert "stream.writeByte (static_cast<char> (juce::jlimit (0, 126, step.velocityBeforeMax)))" in s
assert "step.velocityBeforeMax.store (source.velocity < 127" in s
assert "step.velocityBeforeMax.store (formatVersion >= 7" in s
assert "stream.writeByte (static_cast<char> (juce::jlimit (0, 127, step.noteBeforePause)))" in s
assert "stream.writeByte (static_cast<char> (juce::jlimit (0, 100, step.lengthBeforeLegato)))" in s
assert "reset.noteBeforePause = 60; reset.lengthBeforeLegato = 50; reset.velocityBeforeMax = 100;" in e

# Version number and original GitHub Actions build trigger both intact.
assert "TEST2.4.45-Windows-x64" in workflow
assert "  push:" in workflow
assert "python Tests/check_toggle_memory.py" in workflow
print("PASS: TEST2.4.42 Note/Length/Velocity Enter remembers per-step values; no separate Accent or page changes.")

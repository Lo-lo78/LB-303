"""TEST2.4.60: UIA speech + Pattern/Song context for MIDI Step Entry."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
ed=(r/'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
acc=(r/'Source/ScreenReaderAnnouncer.cpp').read_text(encoding='utf-8')
wf=(r/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
assert 'MIDI Step Entry On. Step ' in ed
assert 'MIDI Step Entry Off' in ed
assert 'End of Pattern. MIDI Step Entry Off' in ed
assert 'Step announcements On. Step ' in ed
assert 'Step announcements Off' in ed
assert 'midiEntrySpeak = false;' in ed
assert 'else if (midiEntrySpeak)' in ed  # MIDI-only automatic speech
assert 'Computer keyboard navigation is ALWAYS spoken' in ed
assert 'finishMidiEntry (false);\n        // Completion must always be spoken' in ed
assert 'NotificationProcessing_ImportantAll' in acc
assert 'preserveOrder' in acc
assert 'processor.songs.setSongMode (songView);' in ed
assert 'processor.songs.setSongMode (false);' in ed
assert 'processor.restartSongPreviewClock();' in ed
assert 'TEST2.4.73-Windows-x64' in wf
print('PASS: LB303 MIDI speech, guaranteed end, UIA and automatic preview context')

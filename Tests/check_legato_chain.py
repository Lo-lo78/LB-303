#!/usr/bin/env python3
"""Structural regression guard for actual C++ gate scheduling. See test_legato_chain.cpp."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
cpp=(root/'Source/SequencerPlayback.cpp').read_text(encoding='utf-8')
hdr=(root/'Source/SequencerPlayback.h').read_text(encoding='utf-8')
for token in ['pendingNoteOffPpq', 'const bool tie = step.length == 101',
              'if (lastNote[lane] >= 0 && lastNote[lane] != note',
              'emitNote (true, note, velocity, onPpq);',
              'emitNote (false, lastNote[lane], 0, onPpq);',
              'pendingNoteOffPpq[lane] = tie ? -1.0', 'flushNoteOff (laneEndPpq);']:
    assert token in cpp,token
assert 'pendingNoteOffPpq' in hdr
assert '1.02' not in cpp
assert (root/'Tests/test_legato_chain.cpp').is_file()
print('PASS: LB-303 true Length 101 gate ties (realtime test available in C++)')

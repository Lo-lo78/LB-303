# LB-303 0.99.2 — TEST2.4.68: determinismo LFO Free con Play della DAW

Data: 5 novembre 2026.

## Funzionamento

- Il VST3 legge lo stato del trasporto dall'host JUCE (`getPlayHead()->getPosition()->getIsPlaying()`).
- Su un passaggio effettivo Stop → Play valido, entrambi i motori ricevono la richiesta di restart del **solo LFO in modalità Free**.
- Per il Free vengono reimpostati fase interna (prima di applicare il parametro Phase), Sample & Hold con seme fisso, smoothing, delay e ciclo One Shot. La modulazione ricomincia in maniera ripetibile a ogni Play, compresa la modalità Sample & Hold.
- Gli LFO in modalità Triggered continuano a essere riavviati dai MIDI Note On esattamente come in precedenza. La modalità Free non diventa Triggered dalle note.
- Quando la DAW non comunica temporaneamente lo stato del trasporto, non viene simulato un evento Stop/Play.
- Non vengono riavviati oscillatori di sintesi, inviluppi, note sostenute, sequencer, delay, chorus, riverbero, distorsioni o preview. Sono invariati ID VST3, preset, Song e scorciatoie.

## Nota sul significato di Free

LFO Free significa che una nota MIDI non ne reimposta la fase; il Play della DAW è un nuovo riferimento temporale. Il normale risparmio CPU di LB-303 è conservato: quando non ci sono voci o destinazioni attive, il generatore LFO può essere sospeso. Perciò questo intervento garantisce il riavvio deterministico, **non** un oscillatore LFO calcolato continuamente a vuoto o un aggancio della sua fase a ogni posizione musicale PPQ.

## Test

`Tests/FreeLfoHostStartTests.cpp` testa il vero `BassEngine` per Free con sinusoide, Sample & Hold, One Shot e il rispetto di Triggered/MIDI. È compilato ed eseguito da GitHub Actions come `LB303FreeLfoTest.exe`; anche il controllo Python `Tests/check_lfo_free_host_start_test2468.py` è nel workflow.

Verificare infine in REAPER e in un'altra DAW (se disponibile) due Play dallo stesso punto con LFO Mode Free e destinazione evidente. Il riferimento è il trasporto dell'host, senza REAPER API proprietarie.

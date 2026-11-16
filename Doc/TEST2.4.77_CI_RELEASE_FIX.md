# LB-303 — TEST2.4.77 — GitHub Actions Release fix

Questa è la **TEST2.4.77**, non la TEST2.4.76. Rimane LB-303 0.99.6 come versione pubblica; NON è una nuova release.

Correzioni rispetto alla TEST2.4.76:

1. `Tests/test_velocity_toggle_state.cpp`: sostituisce `assert(...)` con verifiche `CHECK(...)` che vengono eseguite anche nelle build Release (`NDEBUG`), evitando che istruzioni necessarie alla ricostruzione dei dati vengano omesse.
2. `Tests/check_source_structure.py`: dichiara esplicitamente UTF-8 nelle letture di file di testo, evitando errori di decodifica Windows.
3. `.github/workflows/build-windows.yml`: abilita l'interruzione del workflow sui fallimenti dei comandi nativi PowerShell e imposta `PYTHONUTF8=1`.

**Non cambia il DSP, il sequencer, l'editor Alt+Q, il portamento, gli accenti, il formato dei preset o gli ID dei parametri.** Le modifiche di stabilità audio sono quelle già introdotte nella TEST2.4.76 (vedere `Doc/TEST2.4.76_SEQUENCER_STABILITY.md`).

Per la pubblicazione su GitHub: estrarre **il contenuto** dello ZIP nella cartella del repository che contiene `.git`, `Source`, `Tests`, `.github`. Poi eseguire `git status --short` e controllare che compaiano modifiche prima del commit.

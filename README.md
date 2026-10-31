# LB-303 0.99.1 — TEST2.4.51: reverbero con percorsi spaziali naturali

**Correzione del carattere artificiale del Motion nella TEST2.4.50, senza nuovi parametri o ID VST3.**

- `Left to Right` e `Right to Left` non effettuano più un attraversamento all'84% del ciclo e un ritorno accelerato nel 16% rimanente. Il movimento inverte dolcemente la direzione, senza ripartenza brusca. `Bounce` rimane un'oscillazione più ritmica; `Orbit` mantiene un'evoluzione circolare dei percorsi.
- **Otto percorsi distinti**: la coda viene ascoltata attraverso ritardi aggiuntivi diversi (3,1–17,1 ms), con fasi sfalsate e un lento movimento secondario che impedisce alle riflessioni di ripetersi tutte insieme a ogni ciclo. Questi tap non modificano il feedback FDN.
- **Ombra acustica stereo**: il percorso diretto verso un lato arriva un poco prima; il lato opposto riceve fino a 0,32 ms di ritardo aggiuntivo e parte delle alte frequenze attenuate. Non è un passa-basso sull'intero reverbero e non modifica la bassline dry. I bassi restano relativamente diffusi.
- **Transizioni**: `Motion Depth` sfuma gradualmente fra decodifica originale e ambiente mobile. Cambiando `Motion Mode` verso Off/Expand/Contract il vecchio campo si dissolve senza salto di guadagno. `Expand` e `Contract` continuano a variare solo la larghezza della coda.
- **Compatibilità**: parametri, default Reverb Bass Decay 0,50×, Early Reflections 15%, trim Wet −6 dB, Source Pan 1/2, LFO Pan 1/2, sequencer, automazioni e preset invariati. Con Motion Off o Depth 0 a regime, la somma delle otto linee FDN rimane quella storica.

**Limite esplicito:** il motore FDN è ancora unico e condiviso: le riflessioni hanno tempi/percezioni differenti, ma due note sovrapposte non ricevono due reverberi matematicamente indipendenti. Servirebbe separare ulteriormente le reti, aumentando significativamente il DSP.

La costruzione del VST3 si effettua solo tramite **GitHub Actions**; i test numerici e statici non sostituiscono il giudizio d'ascolto in REAPER.

---

# LB-303 0.99.1 — TEST2.4.50: coda del reverbero realmente in movimento

**Correzione di Reverb Spatial Motion**, senza nuovi parametri VST3. Le modalità `Left to Right`, `Right to Left`, `Bounce` e `Orbit` ora controllano sia le rotazioni interne alle otto linee FDN, sia una **decodifica spaziale dinamica della coda diffusa**: il bilanciamento sinistra/destra della coda cambia in modo percepibile anche dopo il rilascio delle note, non soltanto il timbro delle riflessioni. La lettura avviene separatamente sulle otto linee con un peso differente per ogni percorso, prima della somma con le early reflections e prima del controllo Width. **Non viene applicato alcun autopan al segnale dry né al bus wet complessivo.**

- **Reverb Motion Mode**: `Left to Right` percorre prevalentemente il campo da sinistra a destra, con ritorno breve e smussato; `Right to Left` fa il percorso opposto. `Bounce` oscilla regolarmente fra i due lati. `Orbit` conserva la circolazione interna e modula la posizione della coda. `Expand` e `Contract` modulano invece l'ampiezza stereo della coda durante il suo decadimento, senza imporre un movimento laterale.
- **Reverb Motion Depth** (0–100%, default 60%): controlla anche quanto si sposta la **coda lunga**. A 100%, ai lati, la decodifica FDN privilegia nettamente un canale; a 0% il comportamento è invariato rispetto alla TEST2.4.49.
- **Reverb Motion Rate** (0,02–2 Hz, default 0,35 Hz): velocità del movimento. Per ascoltare chiaramente un attraversamento durante un riverbero breve, prova `1,00 Hz` o più e una coda di 2–3 secondi.

Le transizioni di posizione e di modalità hanno uno smoothing di 30 ms. L'uscita wet mantiene il trim **−6 dB** della TEST2.4.49; **Reverb Bass Decay** di default resta **0,50×**, **Reverb Early Reflections Level** rimane **15%**. Reverb Source Pan 1/2 e Reverb LFO Pan 1/2 continuano a posizionare l'**eccitazione** delle due sorgenti, senza controllare questo nuovo movimento della coda. Off e Depth 0, dopo l'inizializzazione, mantengono il percorso esatto della TEST2.4.49; durante la disattivazione, la posizione torna al centro gradualmente per evitare scatti.

La compilazione VST3 è prevista **esclusivamente tramite GitHub Actions**, non eseguita localmente. La resa sonora finale va verificata in REAPER.

---

# LB-303 0.99.1 — TEST2.4.49: Bass Decay 0.50× e ritorno Reverb −6 dB

## Nuovi valori di partenza del reverbero

- **Reverb Bass Decay**: default **0,50×** (prima 1,00×). L'intervallo resta 0,50–2,00×: a 0,50 i bassi decadono in metà tempo rispetto alla coda dei medi, **all'interno della rete di feedback FDN**, non con un EQ sull'uscita. Il reset tramite Backspace e le nuove istanze usano 0,50×.
- **Reverb Early Reflections Level**: default **15%**, invariato rispetto alla TEST2.4.48.
- **Volume del solo reverbero**: ridotto in DSP di **6 dB esatti**, moltiplicando il ritorno wet finale per `10^(-6/20) ≈ 0,501187234`. Il taglio interessa sia la coda diffusa sia le prime riflessioni, senza ridurre l'energia all'interno dell'FDN. Il parametro **Reverb Level** mantiene la scala 0–100% e il default 50%; non viene modificato il guadagno dei Delay, Chorus, Dry, delle due bassline né dei Reverb Send.

**Compatibilità:** nessun nuovo ID VST3 e nessun parametro rinumerato. I progetti e i preset che hanno già salvato esplicitamente Bass Decay a 1,00× (o un altro valore) continueranno a usarlo: per ottenere 0,50× occorre impostarlo o ripristinarlo manualmente e salvare di nuovo. **La riduzione di 6 dB del reverbero si applica a tutti i preset**, anche quelli già salvati; non cambia invece il parametro Reverb Level memorizzato. Il valore default 15% delle prime riflessioni rimane tale. Nessuna modifica al sequencer, Song, Length 101, automazioni o movimento spaziale. Compilazione solo tramite GitHub Actions.

---

# LB-303 0.99.1 — TEST2.4.48: default Early Reflections 15%

## TEST2.4.48 — Reverb Early Reflections Level, default 15%

**Reverb Early Reflections Level** in **FX → Reverb** now defaults to **15%**
(previously 100%). Its range remains **0–100%**, with 1% steps.
The level affects only the initial stereo reflections, not the bassline dry
signals or the diffuse FDN tail. The early reflections remain equally present
at centre and at the stereo extremes, and panning does not change their volume.
Backspace/parameter reset and new instances use 15%. Saved projects/presets
that explicitly contain another value (including 100%) retain their stored
value; change and resave those if desired. No VST3 IDs, sequencer data,
Parameter Locks or audio routing have been changed.

Compilation is via GitHub Actions, with no local VST3 build.

---

# LB-303 0.99.1 — TEST2.4.47: livello costante delle prime riflessioni

## TEST2.4.47 — Reverb Early Reflections Level

In **Alt+Q → FX → Reverb** è disponibile il parametro **Reverb Early Reflections Level**
(`0–100%`, step 1%, default `100%`). Controlla esclusivamente il livello delle
**prime riflessioni stereo (11/23/34/49 ms)**: a 0% le disattiva e resta il
reverbero FDN diffuso; a 100% usa l'intensità prevista dalla TEST2.4.46
agli estremi del pan. Il livello viene smussato per circa 25 ms durante
l'automazione. Il dry delle due bassline, il volume generale del riverbero,
i Reverb Send e la coda FDN non vengono alterati dal nuovo parametro.

**Correzione essenziale:** nella TEST2.4.46 le prime riflessioni erano di fatto
attenuate o disattivate quando **Source Pan** e **Reverb LFO Pan** erano a zero,
poi emergevano mano a mano che ci si spostava dal centro. Adesso le riflessioni
**sono presenti anche al centro** e il loro livello non dipende più dall'ampiezza
del pan o dell'LFO. Gli LFO e Source Pan regolano soltanto la **posizione**
delle prime riflessioni, usando una distribuzione stereo a potenza approssimativamente
costante. Una compensazione calcolata dai coefficienti dei quattro tap
normalizza anche la loro energia attesa fra centro e lati. Restano possibili
variazioni timbriche dovute ai tempi e alle fasi fra le pareti, ma non viene più applicato un guadagno legato alla posizione.

**Compatibilità:** nuovo ID VST3 `fx_reverb_early_level` aggiunto in fondo al
catalogo, dopo i 4 parametri Source Pan/LFO Pan. Nessun parametro precedente,
Parameter Lock, pattern, Song Editor o preset viene rinumerato. I vecchi preset
ricevono il nuovo default 100%: **il centro suonerà diversamente rispetto alla
TEST2.4.46, intenzionalmente**, perché ora produce le medesime prime riflessioni.
Room, Plate, Hall, Cathedral, Reverb Spatial Motion, Bass Decay/XOver e i due LFO
continuano a funzionare come prima. Compilazione VST3 esclusivamente con GitHub Actions.

**Prova:** Hall, Width 100%, Early Reflections Level 100%, Source Pan 1 = 0,
poi -100 e +100; il livello delle prime riflessioni deve essere presente anche
al centro. Porta Early Reflections Level a 20–40% per dosarle, poi attiva
Reverb LFO Pan 1 per ascoltare il movimento senza sbalzi di volume legati
all'attraversamento del centro.

---

# LB-303 0.99.1 — TEST2.4.46: Reverb Source Pan più incisivo

## TEST2.4.46 — Correzione della posizione spaziale del reverbero

La precedente codifica nella FDN mescolava la posizione sinistra/destra in modo
quasi impercettibile: i canali risultavano quasi uguali anche con Source Pan
agli estremi. **La TEST2.4.46 corregge la codifica direzionale delle sorgenti**
rispetto alla decodifica del motore FDN e introduce **prime riflessioni stereo
per singola bassline**, con quattro tempi diversi (11, 23, 34, 49 ms) oltre al
predelay esistente. La riflessione più vicina è più intensa dal lato della
sorgente, quella opposta arriva con percorsi e intensità differenti. Dopo le
prime riflessioni il campo del reverbero diventa naturalmente più diffuso.

Non è un autopan del wet e non viene spostato il segnale diretto della bassline.
Non sono necessari nuovi parametri: **Reverb Source Pan 1/2** e **Reverb LFO Pan
1/2** già esistenti regolano la posizione e il movimento. A tutti e quattro i
controlli a zero, la TEST2.4.46 manteneva il vecchio percorso senza nuove
riflessioni. **Dalla TEST2.4.47 le prime riflessioni sono invece presenti anche
al centro**, come spiegato sopra. Preset, ID VST3, automazioni, sequencer e
controlli accessibili restano invariati. Il piccolo costo aggiuntivo (solo con
pan/LFO pan attivo) sono letture da buffer di predelay già esistenti.

**Prova:** un solo Reverb Send attivo, Reverb Type = Hall, Reverb Width = 100,
Source Pan corrispondente a -100 e poi +100. Per un movimento continuo,
Source Pan = 0 e Reverb LFO Pan = 100 con un LFO lento a sinusoide.


In **FX → Reverb** sono disponibili quattro nuovi parametri, per Room, Plate, Hall e Cathedral:

- **Reverb Source Pan 1** e **Reverb Source Pan 2**: da **-100 a +100** (default 0). Spostano il punto di eccitazione del reverbero della rispettiva bassline, **senza muovere il suo segnale dry** e senza applicare un pan al ritorno del riverbero.
- **Reverb LFO Pan 1** e **Reverb LFO Pan 2**: da **-100 a +100** (default 0). Regolano quanto gli LFO già esistenti di Bassline 1 e Bassline 2 muovono il rispettivo punto di origine delle riflessioni. Il segno negativo inverte la modulazione. LFO Wave, Rate, BPM Sync, Phase, Mode, One Shot, Smooth e i lock di ogni bassline rimangono quelli della pagina Bassline: non nasce un terzo LFO.

La rete FDN resta **una sola**: ogni send mantiene il proprio segnale stereo attraverso due tap di predelay aggiuntivi per bassline, e l'ingresso al motore FDN viene distribuito su otto linee mediante due piani di rotazioni ortogonali. Non cambia l'uscita wet o la coda già generata: una sorgente spostata immette nuove riflessioni in punti diversi, poi lo Spatial Motion esistente può farle circolare nella rete. Il delay collegato al riverbero mantiene il vecchio routing stereo. Le posizioni sono smussate a 12 ms durante le automazioni; la quantità di energia dell'ingresso di ciascuna bassline è conservata nelle rotazioni.

**Default 0 per tutti e quattro**: la vecchia distribuzione stereo della rete FDN fra linee pari e dispari resta identica (ma nella TEST2.4.47 si aggiungono le prime riflessioni al centro); nessuno degli ID VST3 precedenti è stato spostato, né i Parameter Locks, i pattern, i formati preset, il Song Editor o Length 101. Versione pubblica 0.99.1 invariata. Gli LFO continuano a essere valutati per modulare il send anche quando non hanno altre destinazioni sul synth, ma non spostano il suono dry. Nessun secondo reverbero; memoria aggiunta limitata a quattro piccole linee di predelay.

**Prova suggerita:** seleziona Hall; Source Pan 1 = -60, Source Pan 2 = +60; Reverb LFO Pan 1 = +75 e Pan 2 = -60; modifica i Rate e le Wave degli LFO nelle rispettive pagine Bassline. Porta tutti i quattro valori a 0 per confrontare con la TEST2.4.44. Compilare con GitHub Actions e verificare il suono in REAPER; nessuna compilazione locale.

# LB-303 0.99.1 — TEST2.4.44: movimento interno delle riflessioni del riverbero

Nella pagina **FX**, colonna **Reverb** (Room, Plate, Hall, Cathedral), tre nuovi controlli sono raggiungibili dalla griglia con NVDA/OSARA e automatizzabili in REAPER:

- **Reverb Motion Mode**: `Off` (default), `Left to Right`, `Right to Left`, `Bounce`, `Orbit`, `Expand`, `Contract`. `Bounce` oscilla fra i lati; `Orbit` fa circolare le riflessioni su due piani; `Expand` apre gradualmente la coda dopo l’ultima eccitazione del riverbero; `Contract` la richiude. I due modi direzionali invertono il senso di circolazione delle riflessioni.
- **Reverb Motion Depth**: `0–100 %`, step 1, default `60 %`. Intensità del movimento *interno alla rete*; a 0% resta fermo anche con Mode attivo.
- **Reverb Motion Rate**: `0.02–2.00 Hz`, step 0.01, default `0.35 Hz`. Velocità dei movimenti; in Expand/Contract determina la durata (circa 1/Rate secondi) dell'apertura o chiusura.

**Non è un pan:** vengono rimescolate, durante le ricircolazioni, le 8 linee di riflessione dell'FDN tramite rotazioni ortogonali (a energia preservata) su due piani. Le due uscite wet rimangono decodificate dal solito riverbero e i send/dry non vengono mossi. L'interpolazione delle rotazioni è smussata a 45 ms per ridurre gli scatti. I parametri originali `Reverb Width`, `Bass Decay`, `Bass XOver`, `Damping`, `Level`, `Decay` e `Predelay` restano indipendenti. `Mode Off`, default dei vecchi preset, mantiene *identico* il vecchio algoritmo, senza costo significativo del movimento attivo.

**Compatibilità:** tre nuovi ID `fx_reverb_motion_mode`, `fx_reverb_motion_depth`, `fx_reverb_motion_rate` aggiunti **dopo** `fx_reverb_bass_xover` senza modificare ID esistenti, lock, Pattern, Song, Accent, Length 101 o formati `.lb`. GitHub Actions pubblica il pacchetto **TEST2.4.44**. Il DSP è stato verificato con test indipendenti sul motore; il VST3 completo e l'ascolto in REAPER restano da verificare.

# LB-303 0.99.1 — TEST2.4.43: controllo naturale della coda dei bassi nel riverbero

Nella pagina **FX**, colonna **Reverb**, con Reverb Type su Room, Plate, Hall o Cathedral sono disponibili due nuovi parametri accessibili da tastiera/NVDA e automatizzabili in REAPER:

- **Reverb Bass Decay** `0.50–2.00 x` (step 0.01; default **1.00 x**): moltiplica il tempo RT60 delle sole basse frequenze in ciascuna delle 8 linee di feedback. `0.50` accorcia la coda dei bassi alla metà; `1.00` mantiene invariato il vecchio comportamento; `2.00` raddoppia il tempo di decadimento dei bassi. L'inizio del riverbero e la coda dei medi/alti rimangono presenti; non è un semplice EQ high-pass sul ritorno wet.
- **Reverb Bass XOver** `50–1000 Hz` (step 1 Hz; default **360 Hz**): imposta la frequenza della separazione graduale del decadimento basso rispetto ai medi. Se Bass Decay è 1.00, XOver non cambia la timbrica.

I due controlli operano **dentro la rete di feedback FDN**: filtro passa-basso interno al feedback per separare le componenti, con differenza di guadagno calcolata dai due RT60. Il controllo Damping originale continua ad agire sulle alte frequenze. I coefficienti dell'attenuazione del basso e del crossover cambiano con smoothing (35 ms) per ridurre gli scatti durante l'automazione. La soglia di mantenimento della coda DSP include ora il decadimento basso quando supera quello principale. Nessun cambiamento all'ordine degli FX o al gain del ritorno.

**Compatibilità:** VST3 ID `fx_reverb_bass_decay` e `fx_reverb_bass_xover` aggiunti *dopo* l'ID `fx_oversampling` (non in mezzo ai vecchi FX). Nessun vecchio ID, slot di lock, parametro Sequencer/LFO/OSC2 o formato di salvataggio viene spostato. Preset vecchi utilizzano i valori neutri; preset nuovi salvano entrambi i valori automaticamente tramite APVTS. Le sette pagine Alt+Q, l'Accent via Velocity e il Legato 101 restano invariati. Workflow GitHub Actions: **TEST2.4.43**, nessuna compilazione locale.

# LB-303 0.99.1 — TEST2.4.42: legato 101 continuo su più step

**Alt+Q → Length 101** è ora un vero legame di gate, non una nota di durata 102%. Quattro step consecutivi a 101 mantengono un unico gate per quattro step; 16 step a 101 proseguono senza interruzione anche dall'ultimo al primo. Se l'altezza cambia, il sequencer esegue un passaggio legato senza vuoti; se resta uguale non emette nuovi Note On. Il primo step con Length ordinario stabilisce la fine dell'intera frase. Pause, stop, reset e disabilitazione rilasciano la nota. Velocity 127, invio con memoria, synth, FX, pattern e Song invariati.

Test C++ eseguibile senza JUCE completo: `g++ -std=c++17 -O2 -ITests/Shim -ISource Tests/test_legato_chain.cpp Source/SequencerPlayback.cpp Source/SequencerState.cpp -o test_legato && ./test_legato`. Compilazione e prova VST3 con NVDA necessarie.

# LB-303 0.99.1 — TEST2.4.41: Alt+Q Velocity 127 con memoria per step

- In **Alt+Q**, pagina 3 **Velocity**, **Invio** alterna tra **127** e l'ultima velocity ordinaria scelta sullo step. Esempio: `84 → 127 → 84`. Se lo step non è mai stato modificato, il valore ordinario iniziale è 100.
- La memoria è individuale per step e pattern, è aggiornata quando si regola manualmente una velocity inferiore a 127 e viene mantenuta con selezioni multiple, Undo/Redo, copia/incolla e salvataggio/ricaricamento.
- Le pagine **1 Note** (`nota → Pause → nota`) e **2 Length** (`lunghezza → 101 → lunghezza`) funzionavano già correttamente e restano invariate.
- Il salvataggio delle sequenze passa dal formato **6** al formato **7**. Sono leggibili anche i formati precedenti; per una vecchia velocity già a 127, non essendo disponibile il valore precedente, il primo ripristino utilizza 100.
- Restano sette pagine, con l'Accent originale gestito dalla velocity e da Accent Threshold, senza pagina Accent separata. Nessuna modifica al DSP, ai parametri VST3 esistenti o alla versione pubblica **0.99.1**.
- Verificare la compilazione GitHub Actions e poi Invio/velocità in REAPER con NVDA.

# LB-303 0.99.1 — TEST2.4.40: correzione focus dopo Esc da Alt+H

La selezione della lingua di Help mantiene lo stesso menu a sette lingue. Quando si annulla con Esc, il focus viene restituito al controllo da cui si è aperto il menu: **Grid, Value oppure Edit manuale**. Il ripristino viene eseguito dopo la chiusura effettiva del popup e aggiorna **sia la tastiera sia il focus accessibile Windows UIA/NVDA**, con una seconda verifica differita per prevenire il blocco delle frecce nella Grid e dell'editing. Non serve premere Tab/Shift+Tab per riprendere la navigazione. Non modifica l'operazione di selezione di una lingua e apertura della guida nel browser. Non aggiunge nuovi elementi nel ciclo Tab.

Tutti i parametri VST3, motore audio, preset, Accent, Pattern/Song, oversampling, CPU idle, About e le sette lingue sono invariati. Versione pubblica e data di rilascio rimangono 0.99.1 / 27 October 2026. I controlli statici sono eseguiti localmente; **va ancora collaudato l'effettivo focus Windows UIA/NVDA in REAPER**, oltre alla compilazione VST3 su GitHub Actions.

# LB-303 0.99.1 — TEST2.4.39: correzione lettura UTF-8 nei test GitHub Actions

La TEST2.4.38 si bloccava alla fase `Check source regressions` prima di compilare il VST3: su Windows Python utilizzava la codifica locale cp1252 e falliva nel leggere il sorgente Unicode `Source/PluginEditor.cpp`. In `Tests/check_help_about.py` ora tutte le letture di testo specificano `encoding="utf-8"`. Il workflow e il nome dell’artifact riportano TEST2.4.39. Nessun codice audio o di interfaccia è stato modificato rispetto alla TEST2.4.38; la versione pubblica rimane 0.99.1, release date 27 October 2026.

# LB-303 0.99.1 — TEST2.4.38: Help multilingue e About (27 October 2026)

Partendo dalla TEST2.4.37, come LJuno-116: pulsanti Help e About nell'interfaccia, Alt+H apre il menu della lingua e il manuale HTML incluso nel VST3 nel browser predefinito; Alt+A apre un pannello About leggibile con NVDA (testo di sola lettura, Project, Contact e Close, Tab e Shift+Tab, Esc/Alt+C). Le lingue sono inglese, italiano, spagnolo, francese, russo, cinese e giapponese. About mostra versione 0.99.1, Release date: 27 October 2026, licenza AGPL-3.0-or-later, repository e contatto già documentati nei progetti. Gli ID e lo stato audio, i preset, Song/Pattern, l'Accent, il browser che ricorda solo il caricamento confermato, l'oversampling e il riposo DSP non vengono alterati. La versione del progetto CMake coincide con la versione visibile in About. Il giro Tab dei parametri resta Grid/Value/Edit. La compilazione completa VST3 e la prova con NVDA restano da fare su GitHub Actions / REAPER.

## TEST2.4.37 — Preset browser: selezione persistente tra sessioni

Il browser (Alt+B) apre la cartella e seleziona l'ultimo preset **effettivamente caricato e confermato**, anche nella stessa istanza dopo aver navigato altrove senza premere Invio. La posizione è una preferenza permanente indipendente dai progetti REAPER e dalle altre istanze. Le anteprime, lo scorrimento senza conferma e il semplice salvataggio di un preset non cambiano tale preferenza. Rinomina e cancellazione del preset ricordato ne aggiornano o invalidano il percorso. In assenza di un preset precedentemente confermato e ancora valido, il browser parte dall'inizio della libreria. Non viene caricato alcun suono automaticamente all'apertura del VST.

Nessuna modifica all'audio, all'oversampling, ai preset, alla versione di serializzazione o alle 7 pagine del sequencer.

# TEST2.4.35 — Idle DSP a riposo (base TEST2.4.34)

Ottimizzazioni senza nuovi parametri: i blocchi sonori che non contribuiscono al risultato sono **bypassati**, soprattutto in oversampling 2x/4x. Questa è selezione dinamica del percorso DSP, **non** eliminazione fisica di codice dal VST: la decisione di quale percorso usare richiede comunque un controllo minimo.

- **LFO Bassline 1 e 2:** se Pitch, Volume, Filter e Pan Depth sono *tutte* 0, la forma d'onda LFO non viene generata. Quando una Depth ridiventa diversa da zero, l'LFO riprende dalla fase congelata, quindi non simula retroattivamente i cicli trascorsi mentre era inutilizzato.
- **Osc 2:** con Volume zero e gain già sceso a zero, niente forma d'onda PolyBLEP secondaria, calcolo di detune o avanzamento della fase. Quando viene riattivato il suo gain riparte con il precedente fade-in di 5 ms e si allinea all'oscillatore principale. Lo spegnimento completa la dissolvenza prima del bypass.
- **Distortion 1 e 2:** con Type Off o Amount 0 non viene eseguita la distorsione della bassline interessata. In 2x/4x la scelta avviene prima del ciclo di rendering del segmento (anche dopo uno step lock). Gli stati del distorsore sono ripuliti quando viene spento.
- **Distorsori attivi:** le costanti di Drive, armoniche pari/dispari e centro della curva vengono precalcolate al cambio del parametro, non a ogni sotto-campione.
- **Effetti condivisi:** quando Chorus, Delay e Reverb sono tutti Off, la rete wet viene bypassata in blocco. **Le code rimangono intatte** se gli effetti sono abilitati, anche con send a zero: non si interrompono artificialmente.
- **Sintesi:** glide, velocità LPG, coefficienti accent e accordatura statica vengono precalcolati quando mutano parametri, velocity o oversampling.

**Compatibilità:** nessuna variazione a parametri, Accent, sette pagine Alt+Q, sequencer, Song, stato/preset e workflow di automazione. 1x/2x/4x continuano a funzionare. La TEST2.4.34 ha prodotto lo stesso hash audio della TEST2.4.33, campione per campione, in 24 casi di test (LFO/OSC2 on/off, FX on/off, cambio di stato FX, 1x/2x/4x). Un'accensione dopo una lunga disattivazione può legittimamente partire con fase LFO/OSC2 diversa, perché il generatore non ha girato durante il periodo senza contributo audio.

**CPU:** nelle prove DSP locali con gcc -O3, il caso senza modulazioni né effetti ha mostrato un risparmio indicativo intorno al 30-35% nel percorso di prova 1x/2x/4x; questo **non** rappresenta la CPU totale della traccia REAPER e varia in base a macchina, patch e compilatore. Va misurato nel VST3 finale con scenari comparabili. Compilazione completa VST3, NVDA e ascolto restano da verificare su GitHub/REAPER.

# TEST2.4.33 — Oversampling 1x, 2x, 4x (base TEST2.4.32)

- **FX > Oversampling Basslines and Distortions**: 1x (originale, predefinito), 2x, 4x. Parametro globale VST3 append-only `fx_oversampling`, leggibile da NVDA e automatizzabile.
- Entrambe le bassline eseguono oscillatori, inviluppi, filtro risonante, Soft Clip Drive e **il proprio distorsore** alla frequenza selezionata. Antialiasing 8-poli Butterworth IIR prima di ritornare al sample rate del progetto. Nessuna allocazione nel callback audio.
- Chorus, Classic/Dual Delay e Reverb restano a frequenza originale. Le uscite separate e il master prelevano il medesimo segnale post-distorsione/antialias; i send rimangono isolati.
- 1x usa il percorso DSP originale della TEST2.4.32; tutti i parametri storici, le sette pagine Alt+Q, Accent originale, Song e Pattern/Undo restano invariati.
- L'oversampling IIR a fase non lineare cambia leggermente risposta e fase; **Bit Crush e gli altri distorsori possono cambiare carattere in 2x/4x**. Evitare di cambiare la qualità durante una nota: lo switch può produrre una discontinuità. La latenza non ha un offset di campioni fisso (filtri IIR causali), perciò non viene annunciata una latenza fissa all'host.
- Il test statico e il test standalone del filtro verificano bypass 1x, stabilità e attenuazione fuori banda. **Non ancora ascoltato né collaudato come VST3 in REAPER.**

# TEST2.4.32 — Invio ricorda Note e Length (base TEST2.4.29, SENZA Accent per step)

Questa versione riparte esclusivamente dal sorgente **TEST2.4.29**, senza le modifiche Accent della TEST2.4.30/31. L'Accent originale resta basato sulla velocity e sulla soglia Accent Threshold. Alt+Q mantiene **7 pagine** (Note, Length, Velocity, Repeat, Shift, Launch Step, Parameters); la pagina Parameters resta la 7.

- **Alt+Q > pagina 1 Note:** Invio alterna tra Pausa (128) e la nota MIDI precedentemente usata **in quello specifico step**, per esempio `72 -> Pausa -> 72`. Le pause di uno step mai modificato partono dall'inizializzazione 60.
- **Alt+Q > pagina 2 Length:** Invio alterna tra la lunghezza ordinaria precedente e **101 Legato**, per esempio `37 -> 101 -> 37`. Uno step inizialmente Legato già caricato da un vecchio progetto, senza memoria precedente, usa 100.
- Se più step sono selezionati, ciascuno conserva il **proprio** valore. La memoria si aggiorna quando si modifica normalmente Note o Length con le frecce e si mantiene in Undo/Redo, taglia/copia/incolla step e pattern, preset e progetto REAPER.
- Invio nelle altre pagine rimane invariato: su Parameters (7) apre la scelta dei parametri.
- Salvataggio sequencer formato **v6**: legge anche i precedenti formati v1–v5. Importando v4/v5 viene ignorato il precedente dato Accent per step; in v1–v4 i valori nascosti precedenti a Pausa/Legato non esistevano e non possono essere ricostruiti. Nessun nuovo parametro VST3, nessun cambiamento al DSP o alle Song.
- Workflow GitHub Actions e test statici aggiornati per TEST2.4.32. Compilazione VST3/REAPER e verifica NVDA da eseguire.

# TEST2.4.29 — Delete deseleziona tutti gli step
In Alt+Q, nella vista Steps, **Delete** annulla tutta la selezione di step (anche discontinua), mantiene il focus sullo step corrente e annuncia "All steps deselected" con NVDA. Non modifica note, pause, valori o Undo. Nel Song Editor Delete continua a cancellare gli elementi della Song; nel selettore dei Parameter Locks mantiene le funzioni precedenti. Nella pagina Parameters, con una selezione attiva Delete la cancella; senza selezione Delete conserva la funzione di eliminare il lock corrente.

# TEST2.4.28 — Song loop e modifica relativa degli step selezionati
**LB-303 TEST2.4.28:** correzione avvio delle prime note di ciascun pattern della Song in loop, anche quando il confine cade tra due campioni o il primo step ha Shift negativo; le note successive e lo Shuffle mantengono i tempi originali. Nell’editor Alt+Q le selezioni di step sono ora autoritative (non viene modificato uno step solo focalizzato ma non selezionato). Invio sulla pagina Note inverte note/pausa di ogni step selezionato; Su/Giù modifica relativamente gli step attivi, lasciando intatte le pause. Le pagine 1–5 operano sui selezionati; Parameters pagina 7 resta individuale.

# TEST2.4.27 — Shift+Spazio: selezione singola annunciata anche navigando

Nel Song Editor (Alt+Q, Alt+S) **Shift+Spazio** è l'unica scorciatoia Spazio con modificatore che aggiunge o toglie l'elemento dalla selezione. **Ctrl+Spazio non seleziona più** nulla. Quando anche un solo elemento è selezionato esplicitamente, NVDA annuncia `Selected P2, 12 of 64` durante la normale navigazione su quella riga; l'elenco e la lettura dettagliata con Tab mostrano anch'essi la selezione singola (`1 selected`). Senza selezione esplicita, la riga focalizzata viene comunque usata normalmente da Ctrl+C/X e Delete, senza dire `Selected`. Spazio da solo mantiene il Play/Stop previsto da Alt+P. Tutta la selezione discontinua, le tre operazioni sugli appunti, i preset e l'audio restano invariati.

# TEST2.4.26 — Shift+Spazio seleziona un singolo elemento Song

Dentro **Alt+Q → Alt+S (Song)**, **Shift+Spazio** seleziona/deseleziona esplicitamente l'elemento corrente della colonna attiva. **Ctrl+Spazio** non è più una scorciatoia del Song Editor. Anche una selezione esplicita di **un solo elemento** viene rispettata da **Ctrl+C, Ctrl+X e Delete**, indipendentemente da dove si sposta poi il cursore con le normali frecce. Se non c'è selezione esplicita, tali comandi operano sull'elemento focalizzato come prima. Shift+Spazio annuncia immediatamente `Selected P...` o `Deselected P...`; la navigazione normale annuncia `Selected` anche per un solo elemento selezionato esplicitamente. **Spazio senza modificatori** continua a controllare l'anteprima se Alt+P è impostato su Preview. Nessuna modifica a sequencer, DSP, preset o routing.

# TEST2.4.25 — Ctrl+Freccia su/giù riparato nelle griglie Windows/NVDA

- **Bug risolto:** nella TEST2.4.24 il controllo `mods.isCtrlDown() && !mods.isCommandDown()` non poteva funzionare su Windows: per JUCE il modificatore Command coincide con Ctrl. Ora Ctrl+Su/Giù cambia davvero il Pattern 1–16 da Grid, Value ed Edit nelle griglie Bassline 1, Bassline 2, Sequencer e FX.
- **Focus:** introdotta una gestione di riserva nella finestra principale per gli eventi Ctrl+Su/Giù non consumati dai controlli figli; nessuna seconda finestra e nessuna modifica al ciclo di focus Grid/Value/Edit, NVDA, browser o Alt+Q.
- **Invariati:** Alt+M Pattern/Song, Alt+P Space DAW/Preview, Song Editor, DSP, ID VST3, preset `.lb` e 8 canali audio. Alt+Q mantiene i propri Ctrl+Su/Giù.
- **Verifica:** suite di controlli statici; test di compilazione effettivo su GitHub Actions, poi REAPER+NVDA.

# TEST2.4.24 — Alt+M e Ctrl+Su/Giù anche nelle griglie

- **Alt+M** funziona da Grid, Value ed Edit in tutte e quattro le griglie e alterna
  **Pattern/Song** della stessa istanza. Il selettore è condiviso con Alt+Q:
  passare da una schermata all'altra non perde la modalità. Alt+M non avvia
  automaticamente la riproduzione se era ferma. Quando l'anteprima è già
  attiva, il cambio modalità ne riavvia il clock dal principio, come dentro Alt+Q.
- **Alt+P è indipendente:** in modalità DAW lo Spazio resta destinato a REAPER;
  in modalità Sequencer Preview lo Spazio avvia/ferma il contenuto selezionato
  da Alt+M (Pattern oppure Song), sia nelle griglie sia in Alt+Q.
- **Ctrl+freccia su/giù** in Grid, Value ed Edit cambia subito il pattern
  1–16, senza dover cercare il parametro Pattern. Il comando non cambia il
  parametro focalizzato e rispetta i confini silenziosi. Nei contesti Bassline
  1 e Bassline 2 agisce sulla rispettiva traccia; nella griglia Sequencer segue
  il gruppo di parametri B1/B2 focalizzato, oppure il parametro **Editor Track**
  quando il focus è su quello; nella griglia FX segue **Editor Track**.
- Dopo un cambio pattern, tutti i valori Performance per-pattern visibili
  nella griglia sono sincronizzati immediatamente. Se si è in Edit, eventuale
  valore digitato è confermato prima del cambio; il focus resta in Edit.
- Nessun nuovo parametro VST3, nessun cambiamento di DSP, Song, preset `.lb`
  o comportamento delle scorciatoie dentro Alt+Q.

# TEST2.4.23 — Repeat musicale e Alt+M globale

- **Alt+M** ora alterna **Pattern preview / Song preview** da qualunque punto di Alt+Q: editor Steps, editor Song o selettore Parameter Locks. Non serve più aprire Song con Alt+S. Il tipo di anteprima è distinto dal trasporto Alt+P/Spazio; Alt+M non attiva Play se è Stop. Cambiando tipo l'anteprima, se attiva, riparte dal primo step per evitare una posizione incoerente.
- Nel Song Editor, **Alt+Shift+M** conserva l'ascolto isolato del solo pattern sotto il cursore. Alt+R rimane come comando alternativo per cambiare Pattern/Song.
- **Repeat a tempo come in LJuno:** ogni ripetizione utilizza un intero intervallo musicale determinato dal **BPM Division** del pattern, anziché comprimere tutti i retrigger nella durata di un singolo step. Anche lo **Shuffle** alterna intervalli lunghi/corti su ciascuna ripetizione. Repeat Random sceglie il numero di ripetizioni una volta per step, con scelte diverse ai passaggi successivi.
- La durata di ogni pattern nella Song include tutte le ripetizioni e il Launch Delay: il pattern successivo non scatta più prima che siano terminate. Le due bassline mantengono durate/loop indipendenti. I nuovi calcoli sono condivisi dal player e dal Song locator, per evitare disallineamenti.
- Nessun ID VST3 cambiato. Nessun nuovo parametro, né modifica al formato dei preset `.lb`. È cambiata la **durata musicale** dei pattern con Repeat maggiore di 1: una scelta voluta per ottenere retrigger sincronizzati e non mitragliate.
- Sorgente integrale GitHub-ready, test statici, non compilato in locale. Verificare GitHub Actions, REAPER, NVDA e comportamento audio/CPU.

---

# TEST2.4.22 — Performance per Pattern, Random vivo e Song Editor

- I dieci controlli Sequencer di ogni bassline (BPM Division, Shuffle, Legato, Skip, Note/Velocity/Length/Repeat Random, Octave e Semitone Shift) sono ora **specifici per il pattern selezionato**. Cambiando P1–P16 la griglia aggiorna i valori dal pattern senza alterare gli altri pattern; anche i pattern mai programmati partono dai default. Il player usa questi valori anche nella Song, che calcola separatamente la durata di ogni pattern usando il proprio BPM Division.
- Compatibilità: gli stati salvati prima della TEST2.4.22 vengono migrati, copiando i vecchi valori condivisi nei 16 pattern di quella bassline per conservare il suono preesistente. Le nuove impostazioni individuali sono salvate nello stesso bank `.lb` e nel progetto REAPER. Nessun ID VST3 precedente è stato cambiato.
- Random dinamico ispirato a LJuno (xorshift32 con chiavi per evento e passaggio Song): l'ascolto ripetuto genera nuove variazioni senza registrarle negli step e senza dipendere dalle dimensioni dei blocchi audio. Gate/Velocity usano la distribuzione a più estrazioni di LJuno, il Repeat Random sceglie un nuovo valore per step. Lo stato casuale si rinnova avviando l'anteprima.
- Dentro Alt+Q, Song (Alt+S): **Ctrl+freccia su/giù** modifica immediatamente P1–P16 del solo elemento corrente, come in Steps. Invio ora legge i dettagli della riga e non apre più il selettore. Ctrl+Shift+su/giù mantiene la selezione multipla discontinua.
- Unico archivio consegnato: sorgente completo GitHub-ready, non patch. Compilazione prevista solo in GitHub Actions e test audio/NVDA in REAPER.

---

# LB-303 0.0.2 — TEST2.4.22 Song Performance and Random

**Nuovo in TEST2.4.21:** Song Editor accessibile a due colonne indipendenti,
con riproduzione concatenata P1–P16, selezione anche discontinua, appunti Song,
loop per bassline e Undo/Redo. Tutti i dettagli e le scorciatoie sono riportati
nella sezione TEST2.4.21 in fondo al documento. Questo archivio contiene
l'intero sorgente e il workflow GitHub Actions. Verifica in REAPER/NVDA non ancora
effettuata; nessuna compilazione locale.

## Funzioni TEST2.4.19–20 preservate

This test adds a second oscillator to **each** bassline. Apply the patch to TEST2.4.18, or use the complete repository source archive. GitHub Actions builds the Windows VST3; the source is **not** locally compiled or audio-tested.

### New parameters

- **Osc 2 Volume** (0–100%, default 0%, `b1_osc2_volume` / `b2_osc2_volume`).
- **Osc 2 Detune** (−100 to +100 **cents**, default +10 cents, `b1_osc2_detune` / `b2_osc2_detune`).

The extra oscillator follows the same MIDI note, pitch glide, LFO pitch, Waveform (Saw/Square), PWM, accent, amplitude envelope and filter as the primary oscillator. Its phase is aligned at the start of a non-legato note; it then runs at the slightly detuned frequency to produce beats. Detune = 0 means unison. Both signals are combined **before the shared voice filter**. Osc 2 volume and detune have click-reducing ramps. At volume 0% the original oscillator waveform is unmodified. Note that high levels may increase resonance saturation or output level.

The 4 new VST3 parameter IDs are appended after all existing parameters: legacy project automation and old lock slot numbers remain stable. New per-step lock slots are local 54 and 55. New controls are next to Waveform in the Bassline 1 / Bassline 2 Grid and page-7 parameter picker. Presets `.lb` and REAPER projects capture both per-bassline values. The classic PWM Rate and the dedicated LFO per bassline remain unchanged.

Other features — 16 independent patterns per bassline, Alt+P/Space preview, four stereo output buses, and dynamic FX columns — remain intact.

---

# LB-303 0.0.2 — TEST2.4.16

## What's new

- **Preset browser accessibility:** cursor movement (Up/Down, Page Up/Down, Home/End, cyclic letter search) explicitly announces the newly selected preset or folder to NVDA/OSARA, as in LJuno. Presets are still auditioned by loading their complete `.lb` snapshots; the browser does *not* start an independent preview player.
- **Alt+P in the browser:** toggles whether Space controls the existing sequencer preview or is reserved for REAPER (default). While in Preview mode, Space starts/stops both sequencers; selecting another preset updates the currently audible pattern **only while the same preview is already playing**. Browsing while stopped remains silent. Preview continues when the browser closes if it was already playing; Escape restores the previous sound state, without creating a second transport.
- **Four stereo buses (TEST2.4.18):** Master 1/2 is the complete mix (Bassline 1 + Bassline 2 + shared effects). Separate stem buses: Bassline 1 Out 3/4 including its Distortion 1, Bassline 2 Out 5/6 including its Distortion 2; FX-only wet Chorus / Delay / Reverb on 7/8. Both per-bassline `Separate Output` selectors remain Off / Out 3/4 and Off / Out 5/6, default Off. Those switches enable a *stem tap*; they do not subtract voices from the master, which always carries the full stereo mix as requested. Output FX 7/8 is pure wet without direct basslines. FX Dry Level affects only the master direct contribution, not the individual voice stems or the pure wet FX bus.
- **Important REAPER routing:** configure the track for **8 channels** and map the four VST3 output buses to distinct pin pairs 1/2, 3/4, 5/6 and 7/8. For listening to the complete mix, monitor **only 1/2**. If separately mixing stems 3/4, 5/6 and 7/8, **do not monitor/sum Master 1/2 at the same time** or you will hear intentional duplication of the signals. REAPER decides which buses to route into the audible main send: LB-303 cannot prevent host-side summing if you map multiple buses to the same channels. Off for a stem selector makes its bus silent; an unavailable bus does not mute the master. For normal stereo use, leave both stem switches Off and route 1/2 only.
- **Audio-input passthrough fix:** as this is a MIDI instrument and does not process live stereo input, the master output buffers are explicitly cleared before adding the generated sound, preventing accidental addition of an incoming dry signal.
- Two new **append-only VST3 IDs**: `b1_separate_output`, `b2_separate_output`. The existing parameter IDs, presets, 32 patterns, lock slots and shortcut behavior are retained. The output selector is not a sequence step lock.

**Verification:** source-structure regression tests pass; full JUCE/Windows build, REAPER host bus exposure, NVDA/UIA output and audible routing are **not yet tested**. Build only with GitHub Actions and validate on a blank REAPER project first.

---

# TEST2.4.15 — Appunti del pattern corrente (Alt+Q)

- **Ctrl+Shift+C** copia il solo pattern selezionato (128 step, Start/End, Launch Step, Note/Length/Velocity/Repeat/Shift e tutti i Parameter Locks) negli appunti di Windows.
- **Ctrl+Shift+X** copia e azzera **solo questo pattern**; gli altri 15 della bassline rimangono immutati.
- **Ctrl+Shift+V** sostituisce **solo il pattern attualmente selezionato** con quello negli appunti. Puoi incollare in un altro pattern della stessa bassline, nell'altra bassline o in un'altra istanza del plugin.
- **Ctrl+Z / Ctrl+Shift+Z** annullano/ripristinano soltanto la modifica di quel pattern.
- **Ctrl+C/X/V** continuano a gestire i singoli step, senza cambiamenti.
- **Home/End** cambia traccia; **Ctrl+Su/Giù** cambia pattern in Alt+Q.
- Non vengono copiati Volume, Waveform, gli altri parametri delle bassline, FX, canali MIDI VST3, né i rimanenti 15 pattern.
- Gli appunti precedenti `LB303-TRACK-V1` non vengono accettati; il nuovo formato è `LB303-PATTERN-V1`.
- Nessuna modifica a sintesi, 32 pattern disponibili, preset `.lb` o ID VST3. Compilazione tramite GitHub Actions.

# TEST2.4.13 — Alt+P: DAW / Sequencer preview (Windows VST3)

- **Alt+P** alterna la destinazione di Spazio fra **DAW** (predefinito) e **Sequencer preview**. NVDA annuncia il nuovo modo.
- Con **Sequencer preview**, Spazio avvia/ferma contemporaneamente i due pattern, sia dentro l'editor **Alt+Q** sia nelle quattro griglie Bassline 1 / Bassline 2 / Sequencer / FX, anche in Grid, Value ed Edit. Non occorre tenere un tasto MIDI premuto.
- **Esc o Alt+Q** chiudono solo l'editor degli step. La riproduzione dell'anteprima prosegue se attiva, e può essere fermata con Spazio dopo essere tornati ai parametri.
- Passando con **Alt+P** dalla modalità Sequencer preview alla modalità DAW, l'anteprima si ferma per evitare note in sottofondo prive di un comando di arresto.
- Con destinazione **DAW**, il plugin lascia Spazio non gestito affinché REAPER possa usare il suo comando di trasporto. Il recapito effettivo del tasto da finestre VST3 dipende anche dalle impostazioni di tastiera/focus di REAPER.
- La scelta Alt+P è specifica dell'istanza in uso e **non viene salvata** in preset `.lb` o progetti. Ogni nuova istanza parte in modalità DAW.
- Il motore di sintesi, i due LFO, i 32 pattern, Parameter Locks, il browser preset e gli ID VST3 non cambiano.

---

# LB-303 0.0.2 — TEST2.4.13 | DAW / Sequencer Preview Space transport

This update builds on TEST2.4.11. **Do not extract its patch over an older version.**

- **Volume (historic `slider008`) is now first** in both Bassline parameter grids, immediately before Waveform. The redundant post-synth `Level` control is removed from both accessible grids and from output gain. The historic `b1_level` / `b2_level` parameter IDs are retained as inert legacy slots so previously saved indices are not shifted; their automation no longer affects sound.
- **Two independent LFOs**, one assigned to each bassline. Their VST IDs are `b1_lfo_*` and `b2_lfo_*`, appended after every previous ID. The default depth for each modulation target is 0, preserving the sound of existing presets unless LFO depth is explicitly enabled.
- Each LFO has **Mode** (Free or Triggered), **Rate** (0.125–128 Hz), **BPM Sync**, **Wave** (Sine, Triangle, Saw Up, Saw Down, Square, Sample Hold), **Square PWM** for the LFO's square duty cycle, **Delay**, **Phase**, **Upper Squash**, **Lower Squash**, **Smooth**, **One Shot** and **One Shot Percent**.
- Only four **bipolar modulation depths**: **Pitch Depth** (−48..+48 semitones), **Volume Depth** (−1..+1), **Filter Depth** (−1..+1 normalized cutoff) and **Pan Depth** (−1..+1). All start at 0. The old *PWM Rate* and *PWM Depth* of the oscillator are untouched and remain independent of the new LFO: the new **LFO Square PWM** shapes the modulation waveform, not the oscillator pulse width.
- The LFO parameters appear in the existing two Bassline grids, with the same Grid/Value/Edit accessibility, character navigation, shortcuts and eight rows per column. Page 7's lock picker can now lock LFO parameters independently per step/track, reusing virtual local slots 38–53 without altering old lock numbers 1–37.
- No changes to the four main contexts, sequencer, FX routing, browser, `.lb` format, or 32 patterns. Both LFOs are included automatically in complete preset/project state snapshots.

**Validation:** static regression checks only. GitHub Actions Windows build and REAPER/NVDA audio and focus tests still required.

---

# TEST2.4.11 — Dynamic FX grid and fixed effect order

- The FX page has six stable columns: **Dry Level**, **Distortion 1**, **Distortion 2**, **Chorus**, **Delay** and **Reverb**. This FX-only layout supports varying column lengths (up to 16 controls); all other grids continue to use eight rows per column.
- Off effects show ONLY their Type/On-Off selector. Delay Classic displays only its historical 13 delay controls and sends; Dual Tape displays only its 11 dual controls and sends. Selecting Off collapses the column. The same applies to the two distortions, chorus and reverb.
- Left/right arrows change FX column; up/down change rows within the current FX column; Home/End and PageUp/PageDown stay inside that column. Silent borders remain. Tab still cycles Grid / Value / Edit. The focused parameter is retained by its **VST parameter ID**, with fallback to the FX Type selector if a change hides it.
- Fixed audio order **Distortion → Chorus → Delay → Reverb**. The old Delay Reverb Order automation ID remains loadable for compatibility but no longer affects sound and is hidden from the accessible FX grid; do not create new automation for it.
- All existing preset `.lb` data, parameter/lock IDs and the 32 sequencer patterns are retained. When a preset changes FX types, visible columns refresh automatically. All previous DSP except alternate Reverb→Delay routing is unchanged.
- Build with GitHub Actions; no local compilation/audio runtime verification was performed.

---

# LB-303 0.0.2 — TEST2.4.10 (correzione build Windows)

La TEST2.4.9 falliva in GitHub Actions con errori C2059/C2143
nel file `Source/ParameterCatalog.cpp` a partire dalla riga 79: la
definizione di `seqDisplayOrder` era seguita da una seconda chiusura `}};`
non valida prima della chiusura del namespace anonimo. La TEST2.4.10
la elimina e aggiunge un controllo di regressione specifico. Nessun
parametro, ID VST3, routing DSP, comportamento MIDI, accessibilità o
formato preset `.lb` è stato alterato.

I controlli statici sono passati. Compilazione reale e funzionamento
in REAPER devono ancora essere verificati su GitHub Actions.

---

# LB-303 0.0.2 — TEST2.4.8 (cronologia)

## Dry Level: controllo diretto globale

La griglia FX inizia ora con **Dry Level** (0–100%). Il parametro era
precedentemente chiamato **Delay Dry**, ma non rappresenta il segnale wet del
delay: imposta il volume del segnale diretto di **entrambe le bassline**,
lasciando indipendenti le mandate e i ritorni di Classic Delay, Dual Tape,
chorus e riverbero. Funziona con Delay Off, Classic oppure Dual Tape.

L'ID storico **`slider032` rimane invariato**: preset `.lb`, progetti e
automazioni preesistenti continuano a usare lo stesso parametro; cambia
soltanto il nome visualizzato e la posizione nella griglia FX. Per coerenza
con il nuovo significato globale, REAPER lo presenta come **FX - Dry Level**.
Il vecchio ID nascosto della seconda bassline rimane per compatibilità e
continua a non pilotare la sezione FX condivisa.

Nota: un valore Dry Level pari a 0 rimuove il suono diretto, ma non le mandate
agli effetti. In alcuni vecchi progetti con Delay Off o Dual Tape, il nuovo
comportamento può essere diverso, perché precedentemente la regolazione Dry
funzionava soltanto nella modalità Classic.

Aggiornamento solo staticamente verificato, senza compilazione locale.

---

# LB-303 0.0.2 — TEST2.4.6 (cronologia)

La navigazione viene ricordata **solo nell'istanza VST attualmente in uso**.
Chiudere e riaprire la finestra mantiene griglia, parametro e pagina del
sequencer; una nuova istanza, un nuovo progetto o un riavvio di REAPER non
recuperano questa memoria dai salvataggi. Anche lo step di modifica segue
questa regola. I suoni, gli effetti, i pattern e i preset `.lb` si salvano
normalmente. Vedi la sezione TEST2.4.5 in fondo al documento.

Le sezioni precedenti restano come cronologia dello sviluppo.

## TEST2.4.6 — Grid all'ingresso e Reverb Level x2

- **Ingresso nel plugin (NVDA/OSARA):** quando REAPER passa veramente il
  focus all'editor, LB-303 usa l'evento di focus nativo della ComboBox, **come
  Alt+L**, così NVDA può annunciare il contesto completo: ad esempio
  «Bassline 2. Grid, casella combinata, Waveform, Square, non espanso, Alt+L».
  Tolta la notifica manuale «solo parametro/valore» che copriva l'evento
  nativo. La normale navigazione con le frecce mantiene gli annunci concisi.
  Non viene catturato il focus dalla FX Chain alla semplice creazione.
- **Riverbero:** soltanto il gain del ritorno wet è moltiplicato per 2, dopo
  la rete FDN. Il nuovo Reverb Level 50% equivale al vecchio 100%; il nuovo
  100% equivale al vecchio 200%. Restano identici la scala UI 0–100, gli ID,
  i send, il dry e gli altri effetti. Attenzione a un possibile aumento dei
  picchi/clip se il riverbero viene spinto al massimo.
- I preset `.lb`, i pattern, i lock, la navigazione per istanza e gli ID VST3
  non sono modificati. Come sempre, **solo test statici**, build su GitHub
  Actions e ascolto da verificare con REAPER/NVDA.



## TEST2.4.4 — First Grid focus, remembered contexts and Reverb Predelay smoothing

- **Initial entry from REAPER:** like LJuno, the editor root is a keyboard
  focus container. Only after the host focuses that root, an asynchronous,
  peer-checked transfer focuses the selected Grid and announces parameter +
  value. Constructing the plugin must never grab the FX Chain's keyboard focus.
- **Remember the place:** the four contexts (Bassline 1, Bassline 2, Sequencer,
  FX) remember their selected parameter. The last active context and the
  selections are retained when the editor window closes and reopens; the
  sequencer's last active page (1–7) is remembered across Alt+Q. Alt+Q and Esc
  still return to the originating control.
- **Reverb Predelay:** the read position is continuously smoothed over a 35 ms
  transition when Predelay changes, rather than abruptly jumping 44/48 samples
  per 1 ms. The Cathedral FDN now uses an effective delay length consistent
  with each buffer's maximum size when computing feedback.
- **Important:** this targets confirmed discontinuities in the code. Continued
  periodic clicks at a fixed Predelay value need an audio example for a
  definitive diagnosis. No synth, MIDI, preset IDs or pattern format changed.
- **Suggested REAPER test:** enter the new plugin from FX Chain once, use Grid
  arrows immediately, switch to FX and select Reverb Predelay, leave and reenter
  the GUI, check same context/parameter; Alt+Q 7 Esc Alt+Q should reopen page 7.
  Then sweep Predelay through 9, 17/18, 34/36, 68–72 and 136–144 ms on a
  sustained sound, and hold a value fixed while listening for ticks.

## TEST2.4.1 — Home/End and two independent distortions

- **Value**: Home now sets **maximum**, End sets **minimum**, matching LJuno. Alt+Home/Alt+End already used that mapping and remain unchanged. Home/End in Grid and sequencer keep their navigation actions.
- **FX Distortion 1** and **FX Distortion 2** have separate Type, Amount and Harmonics Even/Odd controls, separate DSP state, and process their respective basslines **before** the shared delay, chorus, and reverb sends.
- Old `fx_distortion_type`, `fx_distortion_amount`, and `fx_distortion_harmonics` IDs belong to Distortion 1, unchanged; the three Distortion 2 IDs are appended and default to Off/0/0. Existing `.lb` presets and automation IDs are preserved.
- Load the patch on top of **TEST2.4** and build via GitHub Actions. Runtime behavior must still be verified in REAPER; no local build was performed.

# LB-303 0.0.2 — TEST2.4.1 Shared FX

## FX condivisi (quarto contesto)

**Ctrl+Tab** cicla Bassline 1 → Bassline 2 → Sequencer → FX → Bassline 1.
Grid, Value ed Edit, ricerca alfanumerica, memoria del cursore, NVDA/OSARA e
preset browser **Alt+B** rimangono invariati. La sezione FX ha un solo DSP
stereo comune: l'audio delle due bassline entra tramite send indipendenti.

- **Delay Type**: Off / LB-303 Classic / LJuno Dual Tape (un solo tipo alla volta).
  Due send separati, uno per ogni bassline. I parametri di Classic riutilizzano
  gli ID preesistenti `slider020`…`slider033` della Bassline 1, mantenendo
  automazioni e lock del vecchio delay B1. Il vecchio gruppo delay della
  Bassline 2 viene lasciato nel VST3 solo per retrocompatibilità con i preset;
  non compare più nelle griglie e non controlla l'audio.
- **Dual Tape**: due tempi/speed ratio, sync al BPM, glide, feedback, tono per
  canale, drive tape e stereo spread; DSP adattato dal Dual Delay di LJuno.
- **Chorus**: stereo con LFO in quadratura, delay frazionario e high-pass sui
  ritorni, con livello, rate, width e due send. Adattato dal chorus di LJuno.
- **FX Distortion**: 12 curve e controllo armoniche pari/dispari, stadio
  globale prima dei send; curve e gain staging adattati da LJuno TEST95.
- **Reverb Type**: Off, Room, Plate, Hall, Cathedral. Un unico riverbero stereo
  FDN a otto linee con predelay, damping, decay, width, level e send separati.
  Queste quattro varianti sono nuove; non sono copie bit-per-bit dei reverb LJuno.

I nuovi controlli FX hanno identificatori `fx_*` append-only e vengono
salvati insieme ai 32 pattern, ai loro Parameter Locks e a tutte le impostazioni
nei preset `Documenti\LB-303\*.lb`. I Parameter Locks bassline rimangono
separati per traccia; i vecchi lock del Classic Delay B1 sono applicati al FX
condiviso, gli eventuali vecchi lock del delay B2 sono mantenuti nei pattern
ma non agiscono più sul DSP.

**Licenze:** le parti adattate da LJuno-116 sono riutilizzate nel progetto AGPL-3.0-or-later.

**Avvertenze TEST2.4**: il motore dei due synth viene ora inviato dry alla
sezione FX; il sound mix dei progetti precedenti può cambiare rispetto a
TEST2.3.2, soprattutto quando entrambi i vecchi delay erano udibili.
La configurazione e la compilazione VST3 devono essere confermate via GitHub
Actions. Non è stata eseguita una compilazione locale né un ascolto in REAPER.

---

# LB-303 0.0.2 — TEST2.3.2: Browser preset .lb (da LJuno-116)

## Preset library

On Windows, presets are stored under **Documents\LB-303**, in individual
**.lb** files. The folder is created on first browse/save. Preset snapshots use
the same full plugin state as a REAPER project: ALL synth/effects parameters
for both basslines (including Level and Pan), sequencer enable switches, both
pattern selectors, configurable MIDI channels (Omni/1–16), both independent
banks of 16 patterns × 128 steps, all step values and all Parameter Locks.
There is no dependency on MIDI files, REAPER projects, or external preset paths.

### LJuno-style preset browser and keyboard navigation

- **Alt+B** opens/closes the preset browser; **Alt+S** saves a preset.
- **Alt+-** and **Alt++** load the previous/next preset from the library.
- In the browser, **Up/Down**, **Home/End**, **Page Up/Down** navigate; selecting
  a .lb file previews the ENTIRE patch. **Enter** commits the selected preset.
- **Escape** (or **Alt+B/Alt+C**) cancels browsing and restores the exact
  parameters and both sequencer banks from before the browser was opened.
- **Enter** on a folder enters it; **Backspace** or **Left** goes up one level.
- Alphanumeric keys cycle presets with the same initial; **Shift+letter**
  searches backward, with silent navigation limits where applicable.
- **F2** renames a preset/folder; **Ctrl+N** creates a folder; **Delete** requests
  confirmation before removing a preset; **Shift+F10** or the Windows Menu key
  shows the context menu.
- Saving an existing filename requires explicit overwrite confirmation.
- Browser folder and row are remembered across sessions/projects.
- When closed, the main interface STILL cycles **Grid → Value → Edit** with Tab;
  opening the plugin does not automatically take keyboard focus from REAPER.

No factory .lb bank is included at this stage. LJuno's legacy factory RPL
presets and its .Ljuno file parser are intentionally **not** imported, because
they are not LB-303 presets. The interface follows LJuno's browsing workflow,
while LB-303 stores its own complete native snapshot.

GitHub Actions compiles on Windows; no local compilation was performed for
this release. Before trusting valuable patches, verify save/load/preview/cancel
and pattern-lock round-trip in REAPER with NVDA.

---

# LB-303 0.0.2 — TEST2.3.1 (MIDI, navigazione e anteprima)

Questa versione mantiene **due banche indipendenti da 16 pattern** e tutti gli ID VST3
esistenti, compresi i numeri dei 37 parametri per bassline usati nei Parameter Locks.
Non modifica il motore sonoro, ma cambia il **solo ordine visibile** dei parametri
nelle tre griglie e nel selettore della pagina 7.

- **Alt+Q** rientra nell’**ultima pagina del sequencer utilizzata**, anche dopo aver chiuso
  l'editor da un'altra pagina. Da dentro l'editor, anche mentre è aperto il
  selettore Parameters, **Alt+Q** chiude come Esc e riporta il focus sul
  controllo originale (Grid, Value o Edit), nella stessa griglia e parametro.
- Bassline 1 e Bassline 2 ricordano separatamente la selezione mentre si usa
  Ctrl+Tab. Il selettore Parameters di pagina 7 ricorda l'ultima posizione
  visitata per ciascuna bassline, anche tornando all'editor successivamente.
- **Griglia Sequencer:** 1) Sequencer Bassline 1 Off/On, 2) Pattern Bassline 1,
  3) MIDI Channel Bassline 1, 4) Sequencer Bassline 2 Off/On, 5) Pattern
  Bassline 2, 6) MIDI Channel Bassline 2, 7) Editor Track.
- I due nuovi parametri **MIDI Channel Bassline 1/2** sono automatizzabili,
  ciascuno con i valori **Omni, 1, 2, ..., 16** (predefiniti 1 e 2).
  Quando entrambe le bassline ricevono dal canale 1, una nota sul canale 1
  può attivare entrambe; con Off/On indipendenti è possibile anche suonarne
  una direttamente e attivare la sequenza dell'altra. In Omni ogni bassline
  ascolta tutti e 16 i canali MIDI.
- I canali MIDI del synth sono configurabili **in ingresso**; le note
  generate dal sequencer in uscita MIDI usano il canale scelto e, in Omni,
  il canale predefinito della rispettiva bassline (1 o 2). Internamente
  restano isolate, anche quando condividono un canale esterno.
- **Spazio nell'editor** avvia/ferma **l'anteprima di entrambi i pattern**,
  contemporaneamente. Home/End cambia soltanto la traccia in modifica,
  senza fermare l'anteprima. C4 resta la nota neutra per la trasposizione.
- Griglie Bassline riordinate: prima Waveform, **Level, Pan, Volume, Cutoff,
  Resonance**, poi i controlli del filtro, inviluppi, portamento, accento,
  risposta, PWM e infine il delay. L'ordine è identico nel selettore
  Parameter Locks; i lock salvati restano associati agli ID originari.
- **Nota storica (TEST2.3.1):** Bassline 2 partiva a 0. Dalla TEST2.4.2 il default è 1.00.

**Verifiche:** test statici Python superati, nessuna compilazione locale.
Compilare usando GitHub Actions e verificare in REAPER: suono condiviso sul
canale 1, Omni, selettore pagina 7, Alt+Q/Esc, preview con entrambi i pattern.

---

# LB-303 0.0.2 — TEST2.3: Parameter Locks in playback and alphabetic search

- Step Parameter Locks (up to 32 per step) are applied to the audio engine at the corresponding step, on either MIDI channel independently, including paused steps. Locks override parameters for the active step, then non-locked parameters return to REAPER's current host-automated base values on the next step. The base VST parameters/host automation are never modified.
- This includes synth Portamento Time/Mode, filter, delay, final Level and Pan.
- Grid and Value: type any letter or digit to cycle through parameter names beginning with that character; Shift+letter/digit cycles backward. Edit retains normal text input.
- Alt+Q > page 7 > Enter parameter picker: same forward / Shift+letter backward cyclic search. No Ctrl+Up/Down track switching in picker. "selected" is announced before already-assigned names.
- All previous TEST2.2.9 two-bank routing, keyboard transposition, Z/X/C/V, shortcuts and parameter IDs are retained.

**Portamento test:** use two different sequenced notes with Portamento Mode set to Always. In Legato mode, slides require overlapping notes, so Portamento Time alone may sound unchanged.

This source is intended for GitHub Actions on Windows. Static checks were run; no local compilation or listening test has been performed.

# LB-303 TEST2.2.9 — Keyboard transpose while Sequencer is On

- **C4 (MIDI note 60) = zero transpose**, as in LJuno: C5 raises the pattern by
  12 semitones; B3 lowers it by one. Pattern steps retain their saved pitches.
- MIDI channel **1 transposes only Bassline 1**; channel **2 only Bassline 2**.
  The two 16-pattern banks and independent Sequencer Off/On switches remain.
- When On, physical keyboard notes trigger and **transpose** the sequence,
  without producing doubled direct notes. With multiple held keys, the last
  pressed key determines pitch. On releasing it, pitch returns to the most
  recently pressed key still held. Changing keys does **not restart** the
  pattern; releasing the last key stops it.
- Note Off uses the actual pitch previously sent at Note On even if a keyboard
  note changes between them, avoiding hanging notes when changing transpose.
- Step-editor Space preview follows the **last physical keyboard note** on the
  corresponding channel (defaults to C4).
- No new parameters or altered IDs. Pattern save format, Grid/Value/Edit,
  screen-reader announcements and Parameter Lock data are unchanged. Lock
  **audio playback** is still deferred to TEST2.3.
- Static tests only; compile on GitHub Actions and verify behavior in REAPER.

---

# LB-303 TEST2.2.8 — MIDI-triggered sequencer mode (per bassline)

The Sequencer parameter grid contains two independent automatable Off/On
switches named **Sequencer Bassline 1** and **Sequencer Bassline 2**. They
reuse the old persistent IDs `seq_b1_enable` and `seq_b2_enable`, respectively,
so existing projects and automation retain their values. Both default Off.

- **Off:** keyboard MIDI notes play the selected bassline directly on MIDI
  channel 1 (Bassline 1) or channel 2 (Bassline 2). The sequencer does not
  generate notes for that lane (except manual Alt+Q preview).
- **On:** pressing a keyboard note on that bassline's MIDI channel **triggers
  its selected pattern** at step 1; the physical note is **not** played or
  forwarded alongside the generated notes. Releasing the final held key stops
  the sequence and releases its generated note. Holding multiple keys keeps
  the sequencer running until the last release. Each lane triggers and stops
  independently. Keyboard notes select timing, not pitch transposition in this
  version.
- The pattern clock follows REAPER's BPM; while stopped, it uses the last
  available host BPM (120 if none is supplied). A note retriggers step 1 when
  all keys had been released. Preview via the step editor still works.
- Other channel messages (such as CC or pitch bend) still reach the relevant
  bassline. MIDI channels 3–16 remain ignored by the sound engines.
- Historical TEST2.2.8: Bassline 2 Level defaulted to 0; TEST2.4.2 now defaults to 1.00.

Only static regression checks are run locally. Build on GitHub Actions, then
try both modes and MIDI channels independently in an empty REAPER project.

---

# LB-303 0.0.2 — TEST2.2.7 (fix compilazione GitHub Actions)

- Risolve errore MSVC `C2248` in `Source/SequencerStepEditor.cpp` alla riga 21:
  `juce::FontOptions` ora riceve esplicitamente `juce::Font::plain` come terzo argomento.
- Non cambia DSP, sequencer, pattern, scorciatoie, identificatori dei parametri o accessibilità.
- Aggiunto un controllo statico per questa regressione; l'artefatto Actions porta TEST2.2.7.
- Il log precedente contiene anche warning non bloccanti di conversione e signedness.
- Non è stata eseguita una compilazione locale; verificare su GitHub Actions.

---

# LB-303 0.0.2 — TEST2.2.5 (parameter picker can change bassline)

- **Page 7 — Parameters** (zero-based code index `6`): on a selected step, **Enter** opens a separate accessible selection view listing **37 parameters of the current bassline only** in columns of eight rows. Page 6 remains Launch Step.
- In the selector use Up/Down (rows), Left/Right (columns), Home/End (column edges), Ctrl+Home/End (first/last), PageUp/PageDown (five rows), or initial letters to find parameters. **Enter** assigns/selects one of up to 32 locks for the step and returns to Parameters; **Alt+Enter** adds/removes the highlighted lock without closing the selector; **Esc** returns without changing selections. Boundary keys are silent.
- On the Parameters page, Up/Down edits the highlighted lock value using the parameter's documented range/step, Alt+Home/End changes current lock, Delete removes one, Alt+Delete clears all, Backspace resets one to its initial value. Changes participate in the existing step Undo/Redo and are stored in the pattern bank; they never modify the APVTS base parameter values. Lock playback in the audio engine is **not yet enabled** (TEST2.3).
- All 32 patterns are independent and pattern selectors remain automatable. No synth DSP modifications, parameter ID changes, or new requirements for local compilation. Only static checks have run: GitHub Actions and NVDA/REAPER test pending.

---

## TEST2.2.3 — Tab follows the active Start/End Step range

In Alt+Q, the current pattern on the current bassline has a separate playback
range: **Z** decreases Start Step, **X** increases Start Step, **C** decreases
End Step, and **V** increases End Step (1–128), matching LJuno. This range is
saved with that pattern. If Start=1 and End=32, **Tab** traverses only the
1–16 and 17–32 blocks; at the final block Tab is silent, and Shift+Tab is
silent at the first block. Different patterns on Bassline 1 and Bassline 2
retain independent limits. The editor also does not select or modify steps
outside the active range, and automatically moves selection into range when
Start/End, track, or pattern changes. Editing the range never erases step data
outside it; lengthening the pattern exposes those steps again.

All existing shortcuts remain: Home/End choose track, Ctrl+Up/Down choose the
current track's pattern, and Ctrl+Tab remains unassigned inside Alt+Q.
No synthesizer DSP or VST parameter IDs changed. Static checks only: compile
using GitHub Actions and test behavior with NVDA/REAPER.

---

# LB-303 — VST3 0.0.2 TEST2.2.3 (Active step range)

**Two independent pattern selectors:** Pattern Bassline 1 (1–16) and Pattern Bassline 2 (1–16). Selecting a new pattern in either lane never changes the other. There are **32 patterns total** (16 per bassline), each with 128 steps and up to 32 parameter-lock slots per step. Both use REAPER's transport clock, but have independent pattern lengths, playback positions and note events. Bassline 1 outputs MIDI channel 1; Bassline 2 outputs MIDI channel 2.

Use **Ctrl+Tab** to open the Sequencer parameter grid (Bassline 1 → Bassline 2 → Sequencer). This grid has **Pattern Bassline 1, Pattern Bassline 2, Sequencer Bassline 1, Sequencer Bassline 2, Editor Track**. Navigation remains Grid/Value/Edit with silent bounds and NVDA announcements. The two Play controls default to Off, so existing projects remain unaffected. in this historical TEST2.1 version, Bassline 2 Level defaulted to 0 (now 1.00).

**TEST2.2:** Alt+Q opens the accessible step editor and can now program both
independent 16-pattern banks: Note, Length, Velocity, Repeat, Shift and Launch Step.
The seventh page shows Parameter Lock counts only; adding/applying locks is
planned for TEST2.3. Pattern content is retained when a project is reopened.
All 32 new patterns start as rests until you program them. GitHub Actions
compilation and actual testing with NVDA/REAPER have not yet been performed.

---

# LB-303 (VST3) — version 0.0.1 TEST1.7

Accessible dual-engine bassline synthesizer for REAPER, ported from **LB-303.jsfx**
by Lo-lo. This TEST1 source is prepared for Windows x64 VST3 builds using **GitHub Actions**.

Repository: https://github.com/Lo-lo78/LB-303

## TEST1 scope

- All **35 LB-303 JSFX parameters** retain their original order, values, defaults
  and meaningful ranges. Persistent VST parameter IDs are `slider001` … `slider035` for Bassline 1, with independent `b2_slider001` … `b2_slider035` for Bassline 2. Each has an additional output Level and Pan.
- PolyBLEP saw/pulse, PWM, portamento, velocity/accent, filter ADSR,
  Lowpass Gate, resonant two-stage filter, fixed high-air peak, post-filter,
  saturation and input passthrough have been translated to C++.
- Delay includes tempo divisions, ping-pong, spread/Haas, RBJ LP/HP,
  modulation, dry/wet, feedback and transpose.
- REAPER project state uses JUCE APVTS XML. Sample-accurate MIDI note scheduling.
- Two independent accessible Bassline grids, each **five columns with eight rows per column**.
  No page combo box. Starting with TEST1.3, a single LJuno-style accessible
  ComboBox selects parameters; a dedicated Value slider and numeric Edit field
  replace the 35 individual buttons.
- GitHub Actions will build and upload an installable VST3 bundle.

### Important TEST1 qualification

**Audio equivalence has not been validated yet.** Do an A/B listening test in REAPER
against the included original JSFX, with matched parameter values and velocity.
The delay transposition is initially implemented with a real-time dual-grain
crossfading algorithm. The original JSFX uses **Saike's WSOLA+autocorrelation
pitch shifter**; this effect is therefore not guaranteed to sound identical,
especially on transients. The original JSFX and included libraries are kept in
`Reference/` so the exact algorithm can be transplanted in a later fidelity pass.
The default `Delay Transpose = 0` bypasses the difference.

Other behaviors requiring close listening: the original JSFX has some surprising
implementation details, including half-frequency oscillator phase increments,
accent envelope renewal and a 1.25x free delay time scale. TEST1 intentionally
keeps them, rather than silently modifying the character.

### Keyboard navigation

| Keys | Action |
| --- | --- |
| Up / Down | Previous / next parameter in the same column |
| Left / Right | Previous / next column (8 parameter positions) |
| Alt+Up / Alt+Down | Increase / decrease parameter value |
| Alt+Shift+Up / Alt+Shift+Down | Change by 10 selected steps |
| Alt+Left / Alt+Right | Change the step width: 1, 5, 10, 15, 20 (announces `Step 5`, etc.) |
| Alt+PageUp / Alt+PageDown | Increase / decrease by 40 selected steps (choices change by 1) |
| Alt+Home / Alt+End | Set selected parameter to maximum / minimum, as in LJuno |
| Enter (Grid) | Focus Value for the selected parameter |
| Enter (Value) | Return to the same parameter in Grid |
| Tab / Shift+Tab | Cycle Grid, Value and Edit (both directions) |
| Enter (Edit) | Commit typed value and return to Grid |
| Home / End (Grid) | First / last parameter in the current column |
| Home / End (Value) | Minimum / maximum value |
| PageUp / PageDown (Grid) | Up / down 5 rows in the same column |
| PageUp / PageDown (Value) | Change by 40 selected steps |
| Left / Right (Value) | Change the step width, as in LJuno |
| Ctrl+Home / Ctrl+End | First / last parameter |
| Alt+L / Alt+V / Alt+E | Focus Grid / Value / Edit |
| Alt+Q | Open the accessible two-track step editor (TEST2.2) |
| Ctrl+Tab | Cycle Bassline 1 → Bassline 2 → Sequencer → Bassline 1 |

The focused parameter name and value are also set as native accessibility text,
so screen-reader line reading has a concise value. Host interception of global
shortcuts may vary; please test Alt+L and arrow focus with NVDA/OSARA in REAPER.

## TEST1.1 — GitHub Actions build compatibility fix

- Fixes JUCE 8.0.15 / MSVC error C2039 in `Source/PluginProcessor.cpp`:
  `juce::Optional<double>` has no `value_or()` method.
- Host BPM now uses the same `if (const auto bpm = position->getBpm())`
  and `*bpm` idiom already present in LJuno-116.
- No DSP, parameter IDs, presets, UI or keyboard controls have been modified.
- Static source checks only; compilation is reserved for GitHub Actions.

## TEST1.2 — REAPER editor-opening crash fix

- **Fixed a reproducible invalid pointer dereference in the UI constructor.**
  The previous editor called `setResizeLimits(720, 390, ...)` before constructing
  the 35 `ParameterCell` components. JUCE may resize the window synchronously,
  invoke `resized()`, and trigger a null-pointer access while opening the editor.
- All 35 cells are now created before `setResizable`, `setResizeLimits`, and `setSize`.
  `resized()` also checks for null children as a defensive measure.
- Added static regression checks for the constructor ordering and null guard.
- No changes to DSP, parameter IDs, stored state, navigation, or screen-reader
  announcements; GitHub Actions must recompile, and REAPER needs a fresh runtime test.
- To verify: test the fixed VST3 on a blank REAPER project first. Remove the
  old `LB-303.vst3` directory before installing the updated VST3 directory.

## TEST1.4 — LJuno step-width keyboard shortcuts

- Alt+Left/Right cycle **1, 5, 10, 15, 20** and announce only when the setting changes.
- Alt+PageUp/PageDown adjust by **40 selected steps**; enumerated choice parameters move by one choice.
- Alt+Home takes the selected value to its **maximum**; Alt+End takes it to its **minimum** (LJuno convention).
- Value Left/Right and Value PageUp/PageDown mirror LJuno, also without Alt.
- From TEST2.4.5, the step width is remembered only by the current running instance, not stored in the project/preset.
- Silent borders remain unchanged; all shortcuts work in Grid, Value and Edit without stealing focus on plug-in insertion.
- Only the editor, tests and documentation are changed; DSP and the 35 persistent parameter IDs are untouched.
- **Static checks only; no local build.** Compile on GitHub Actions and test in REAPER.

## Build on GitHub Actions

1. Create a public (or private) repository named `LB-303`.
2. Upload the **contents of this archive** to the repository root. Keep the
   `.github/workflows/build-windows.yml` hierarchy.
3. Open **Actions → Build LB-303 Windows x64**. The `push` starts the build,
   or use `Run workflow` manually.
4. Download the `LB-303-0.0.2-TEST2.2.5-Windows-x64` artifact after it succeeds.
5. Extract the VST3 directory under the usual VST3 folder, e.g.
   `C:\Program Files\Common Files\VST3\`, and scan in REAPER.

No local compilation was performed during source preparation. The first
GitHub Actions run is the actual compiler check.

## Upcoming roadmap (not implemented in TEST1)

- **TEST2:** two-track LJuno-style sequencer, 16 patterns/128 steps, second grid
  for sequencer-wide settings, Alt+Q step editor (Note, Length, Velocity,
  Repeat, Shift, Launch Step, Parameter Locks), copy/paste/undo.
- **TEST3:** precise MIDI note overlap, portamento/legato, accent,
  envelope retriggering and lock interpolation in sync with REAPER.
- **TEST4:** preset/pattern persistence, focus regression tests, MIDI and host
  automation compatibility.

## Attribution and licensing

This repository's new VST3 sources use AGPL-3.0-or-later, matching the LJuno-116
source license. The reference `qddelay` JSFX helper files contain components by
Olivier Belanger (CookDSP permission notice), Theo Niessink (WTFPL v2), and
Joep Vanlier (Saike Pitchlib, MIT); their original notices are preserved.
Reference files are not compiled directly into the VST3.

## TEST1.3 — LJuno-style parameter controls and REAPER focus

- Replaces the initial grid of 35 JUCE TextButtons with **one accessible
  ComboBox** (LJuno-style parameter grid), one APVTS-attached **Value** slider,
  and one separate native **Edit** control for typing numerical/choice values.
  The on-screen grid is still drawn in 5 columns of 8 rows for mouse users,
  but is not made of 35 separate focusable buttons.
- **Tab** cycles Grid → Value → Edit → Grid; Shift+Tab goes backwards.
  **Enter** on Grid focuses Value; Enter on Value returns to the currently
  selected Grid parameter. Enter on Edit commits and returns to Grid.
- **Alt+L / Alt+V / Alt+E** provide direct focus. Grid arrow keys navigate
  by row or 8-item column; Alt+Up/Down edits values. Home/End, page navigation,
  Ctrl+Home/End, type-to-jump, Backspace reset and silent navigation limits.
- Accessibility names follow LJuno's **entry context only** design: Grid and
  Value announce their role on entry but omit repetitive descriptions for
  reading and navigating. The root editor is ignored by the accessibility tree.
- **No automatic focus transfer** when REAPER creates/opens the FX editor.
  The plug-in cannot force REAPER to keep its window closed; that is a host
  preference. This change prevents LB-303 itself from moving the keyboard
  focus away from the host FX Chain during initialization; on actual keyboard
  entry TEST2.4.4 focuses the remembered Grid after the host has focus.
- The 35 original parameter IDs, DSP, JUCE version and Actions workflow
  remain unchanged. Compile and runtime testing remain GitHub/REAPER tasks.

## TEST1.5 — Spoken value shortcuts from Grid, Value and Edit

- Fixes a missing accessibility announcement when Alt+Up/Down, Alt+PageUp/PageDown,
  Alt+Home/End or other parameter-changing shortcuts are used with **Grid** or
  **Edit** focused. The previous implementation only sent a valueChanged event
  when the **Value** slider itself had keyboard focus.
- Announces the **resulting parameter value only**, using the same Windows UI
  Automation notification mechanism as LJuno. Applies also in Value.
- The operation is silent when the resulting value is unchanged at a minimum,
  maximum or due to range/step snapping (silent boundaries remain intact).
- An Alt value shortcut while Edit is focused also updates the edit field to
  the actual new value and selects the text for further editing.
- Restores unobtrusive accessibility descriptions after speaking; does not
  move focus, change the audio engine, change keyboard mappings, or change any
  of the 35 parameter IDs.
- Static regression tests pass; compilation still takes place exclusively
  through GitHub Actions.

## TEST1.6 — Two independent basslines, Level/Pan, three contexts

- Two real `BassEngine` instances, each with its own oscillator, phase, envelopes,
  filter, accent, delay and internal state. No shared synthesis state.
- Each bassline retains the original 35 sound parameters, plus **Level** and
  **Pan** (37 controls per grid, five columns × eight rows). There are 74 VST3
  parameter IDs total, grouped under Bassline 1 / Bassline 2 in the host list.
- Bassline 1 preserves the existing 35 automation IDs exactly. Bassline 2 uses
  stable `b2_slider001` through `b2_slider035` IDs; output controls use
  `b1_level`, `b1_pan`, `b2_level`, `b2_pan`.
- `Volume` is the original pre-filter amplitude control, untouched; `Level` is
  an additional **final** post-delay level. Pan: -100% left, 0 centre, +100% right.
  Linear balance panning keeps the original Bassline 1 signal unchanged at centre.
- Historical TEST1.6 defaults were Bassline 1 = 1.00, Bassline 2 = 0.00. In TEST2.4.2 both factory defaults are 1.00; saved projects keep their values.
- In TEST1.6, incoming MIDI was layered to both basslines. Starting in TEST1.7,
  Bassline 1 receives MIDI channel 1 exclusively and Bassline 2 channel 2.
  The future two-track sequencer will address them independently. TEST1.7 still
  did not contain a functional sequencer engine, clocks or patterns (now added in TEST2).
- `Ctrl+Tab` rotates **Bassline 1 → Bassline 2 → Sequencer → Bassline 1**.
  The Sequencer context displays a descriptive placeholder without bogus
  editable controls. Earlier TEST2.1 still reported that step editing was not yet
  implemented. The two bassline grids remember their selections separately.
- Grid → Value → Edit, Enter, Alt+Left/Right, Alt+PageUp/PageDown, Alt+Home/End,
  silent borders, NVDA/OSARA speech and no-focus-steal-on-load are preserved.
- **No local compilation.** GitHub Actions is responsible for the VST3 build;
  please test both engine outputs and UI focus using NVDA in REAPER.

## TEST1.7 — Independent MIDI channels

- **Bassline 1, channel 1:** receives only MIDI on channel 1.
- **Bassline 2, channel 2:** receives only MIDI on channel 2.
- Note On, Note Off and any channel MIDI messages are routed to the matching
  bassline only; messages on channels 3–16 or channel-less system events
  are not routed into either bassline engine.
- REAPER can host both basslines as one VST3 instrument: send MIDI 1 for the
  first instrument and MIDI 2 for the second. MIDI output is passed through.
- Historically Bassline 2 Level defaulted to **0.00**; from TEST2.4.2 it defaults to **1.00**, except when restored from a saved project or preset.
- No synthesis algorithms, parameter IDs, keyboard controls or UI navigation
  have been changed. Only MIDI event routing, tests and build artifact naming.
- Static checks only. GitHub Actions must compile; REAPER must verify
  channel isolation, including overlapping notes and Note Off messages.


## TEST2.2 — accessible two-track step editor

Press **Alt+Q** while focused in Grid, Value or Edit. The current Bassline
context chooses the track; from Sequencer context the independent **Editor
Track** parameter chooses it. **Esc** returns to the exact control used to
open the editor. Loading the plug-in still does not steal focus from REAPER.

Within the editor:

- **1–7**: Note, Length, Velocity, Repeat, Shift, Launch Step, Parameters.
- **Q W E R T Y U I / A S D F G H J K**: select one of 16 visible steps.
- Double-press a step key to select/deselect it for multi-step edits.
- **Tab / Shift+Tab**: move between the eight blocks of 16 steps (128 total).
- **Home / End**: previous/next bassline editing track (Home selects Bassline 1; End selects Bassline 2). Silent at boundaries. Ctrl+Tab has no action inside the step editor; outside it still switches the three parameter grids.
- **Ctrl+Up / Ctrl+Down**: next/previous pattern (1–16) for the **currently edited bassline only**; the other pattern stays unchanged. The new selection is announced to NVDA/OSARA and updates the corresponding host-automatable VST3 parameter.
- **Up/Down**: edit step; **Left/Right**: increment width 1,5,10,...40;
  **PageUp/PageDown**: move by chosen increment.
- **Enter on Note page**: toggle Pause / C4 (note 60); useful to program an initially empty pattern.
- **Backspace**: reset musical properties of selected steps (rest = MIDI note 128).
- **Ctrl+C, Ctrl+X, Ctrl+V**: copy/cut/paste one or selected steps;
  **Ctrl+Z / Ctrl+Shift+Z**: undo/redo (50 actions).
- **TEST2.4.13 replaces the original Space shortcut:** default Space belongs
  to the DAW. **Alt+P** selects Sequencer preview, after which Space plays/stops
  both lanes even outside the step editor; closing the editor no longer stops it.
  Existing MIDI 1/2 routing is kept.
- **Alt+I**: read track, pattern, page, step and current playback range.

All 32 patterns are independent and are stored in the REAPER project via the
existing TEST2.1 state format. **Page 7 Parameters** displays lock counts,
but Parameter Lock assignment and playback are intentionally reserved for
TEST2.3. The synth audio path and original parameter IDs are unchanged.

Compiled only through GitHub Actions; no local build or REAPER audio test performed.


## TEST2.2.1 — separate track and pattern shortcuts

This release changes **only** the keyboard behavior in the sequencer step editor:

- **Home** moves to the previous bassline (only two: 1 and 2); **End** to the next.
  At Bassline 1 with Home and Bassline 2 with End the navigation is silent.
- **Ctrl+Up** selects the next pattern 1–16 of the edited bassline; **Ctrl+Down**
  the preceding pattern, with silent limits. The other bassline's pattern
  selection is untouched.
- Both **Pattern Bassline 1** (`seq_b1_pattern`) and **Pattern Bassline 2**
  (`seq_b2_pattern`) are already present in the **Sequencer** Grid/Value/Edit
  parameter context (reach it with Ctrl+Tab from Bassline 1, then Ctrl+Tab
  from Bassline 2) as two separate JUCE AudioParameterChoice VST3 parameters.
  They can be automated independently in REAPER, and the editor shortcuts
  update the **same** automatable parameter, not a private editor value.
- The two pattern banks still hold 16 patterns each (32 total); MIDI routes to
  channels 1 and 2. Synth DSP, parameter IDs, project persistence, editor
  focus, Grid/Value/Edit, and Parameter Locks roadmap are unchanged.
- The matching **LJuno-116** shortcut change is *proposed for a future update*;
  it is not part of this LB-303 patch.

This source has static checks only: compile using GitHub Actions and check the
keyboard/screen-reader behavior in REAPER before considering the change verified.


## TEST2.2.5 — Assign locks across both basslines without exiting the picker

In Alt+Q step editor, press 7 for Parameters, then Enter for the picker:

- **Ctrl+Up / Ctrl+Down while the picker is OPEN:** switch to Bassline 2 / Bassline 1. This does **not** change either pattern; each lane retains its own pattern selector (1–16) and step bounds.
- **Alt+Enter:** toggle the highlighted parameter lock **in the current bassline** and keep the picker open. Repeat to assign multiple locks. If the same parameter is already locked, Alt+Enter removes it (as in LJuno).
- **Enter:** select/add the highlighted parameter lock in the current bassline, then close the picker, returning to page 7. If that lock is already present, Enter simply selects it and closes; it does **not** add a duplicate.
- **Esc:** close the picker without deleting locks already assigned.
- **Ctrl+Up / Ctrl+Down when picker is CLOSED:** unchanged; change pattern 1–16 of the current bassline. **Home/End** in the step editor still switch tracks. All boundary attempts remain silent.

For example, add three locks on Bassline 1 using Alt+Enter, Ctrl+Up to Bassline 2, add two locks with Alt+Enter, then Enter while an already-assigned parameter is highlighted. The result is **3 locks on Bassline 1 and 2 on Bassline 2**, both on their respective currently selected patterns and step positions. The picker remembers the selected step separately for each lane, clamped to that lane's Start/End range, and announces bassline, pattern, step and parameter when switching. Undo/Redo and project-state storage remain independent per lane/pattern/step.

**Parameter Lock playback is still deferred to TEST2.3.** Static tests only; GitHub Actions and real NVDA/REAPER tests still required. LJuno itself is not modified.

## TEST2.2.6 — Simplified parameter picker and selected announcements

This release supersedes the **picker-only** Ctrl+Up/Down track switching added
in TEST2.2.5. In the open picker, Ctrl+Up/Down now has **no action** (silently
consumed); it cannot switch basslines or accidentally adjust a pattern. Choose
Bassline 1 or 2 with **Home/End in the step editor before opening the picker**.
With the picker closed, Ctrl+Up/Down still changes the independent 1–16 pattern
of the current bassline, exactly as before.

On page 7 (Parameters), Enter opens the picker. Any parameter **already assigned
as a Parameter Lock to the current step** now has the literal prefix **`selected `**
BEFORE its name, both in the grid and in the NVDA/OSARA announcement, matching
LJuno's convention. Non-assigned parameters are read by name only. This status
updates immediately when Alt+Enter toggles the lock, without closing the picker.
Enter adds/selects and closes; Escape closes without discarding locks already
added. The separate 16-pattern banks and saved parameters are unchanged.

Parameter Lock **playback** is still planned for TEST2.3. Only static checks
have been run; build on GitHub Actions and test announcements in REAPER/NVDA.


## TEST2.4.2 – preset focus, MIDI access, initialization

- Enter on a preset loads it and closes the browser, restoring the original page, selected parameter and Grid/Value/Edit focus. Escape cancels a preview.
- Each Bassline grid also exposes the **same** MIDI Channel Bassline 1/2 VST parameter already in Sequencer (Omni or 1..16), with the same parameter IDs. No duplicated channels or automation.
- Both Bassline Level factory defaults are **1.00**. Already-saved REAPER projects and `.lb` presets preserve their values.
- **Alt+I** from Grid/Value/Edit initializes both basslines, the shared FX, MIDI input settings, both 16-pattern banks (including locks) and sequencer settings; audio tails are cleared on the next audio callback. This destroys unsaved changes: save a `.lb` first. Alt+I inside the step editor retains its existing information announcement.
- Compile only via GitHub Actions. Static checks do not replace REAPER testing.


## TEST2.4.3 – Restore Grid keyboard and NVDA focus after preset load

- Confirming a `.lb` preset with **Enter** closes the browser and returns to
  exactly the same page, parameter, and Grid / Value / Edit control as before.
- When the originating control was **Grid**, a delayed, post-key-event focus
  handoff to the native ComboBox and its accessibility handler prevents the
  browser list from retaining navigation. **Arrow keys work immediately**.
- NVDA explicitly announces the restored parameter name **and loaded value**.
  Normal Grid line-reading behaviour resumes after the announcement.
- A quick deliberate Tab to Value/Edit is respected: delayed recovery will
  not steal focus from those controls. No auto-focus when REAPER opens the VST3.
- DSP, MIDI, FX, patterns, Parameter Locks, `.lb` format, VST3 parameter IDs
  and existing keyboard shortcuts have not changed.
- This is a static-checked source patch; compile with GitHub Actions, then
  test in REAPER with NVDA. Test loading from all four Grid contexts and
  also from Value/Edit to confirm those controls still return normally.

## TEST2.4.5 — Editor navigation is per-instance only

- The active Grid (Bassline 1, Bassline 2, Sequencer, FX), the selected
  parameter in each Grid, the Alt+Q sequencer page and the parameter step
  width are stored **only in the live processor instance's RAM**.
- Closing and reopening the **editor window of that same VST instance**
  restores its position and last sequencer page. Two instances have
  completely independent histories.
- A fresh VST instance (for example, after removing/reinserting it or
  reopening a project in REAPER) starts on Bassline 1, first parameter,
  Note page (and default value step).
- Saving a REAPER project or a `.lb` preset **never saves editor navigation**.
  Old transient editor properties from TEST2.4.4 are stripped on load/save,
  while all audio parameters, 32 pattern banks and Parameter Locks persist.
- Loading a preset into the same running instance leaves its navigation
  unchanged, including browser Enter's return to Grid/Value/Edit.
- Initial focus handling and reverb predelay smoothing from TEST2.4.4
  remain unchanged. No local build was performed; compile on GitHub Actions.


## TEST2.4.7 — Reverb return boost (+18 dB)

- The user found the TEST2.4.6 shared reverb still very quiet at realistic
  settings such as **Reverb Send = 20%** and **Reverb Level = 50%**.
- The output gain of the *wet return only* is multiplied by **8**, or
  **+18.06 dB compared with TEST2.4.6**. This is 16x the wet level of
  TEST2.4.5 because 2.4.6 had already doubled it. Level and Send retain
  their original 0–100% ranges, VST parameter IDs, defaults, automation and
  preset values. No changes to FDN feedback, Decay, Predelay, Width, Damping,
  Bassline dry audio, delay, chorus or distortion.
- At Send 20%, Level 50%, the new wet return is about **8x louder in amplitude**
  than TEST2.4.6 at those settings. Beware clipping with both Send and Level
  near 100% or very hot inputs; reduce levels if the result overloads.
- Static checks only; build with GitHub Actions and listen in REAPER at a
  variety of source levels and reverb types. No local VST compilation.

## TEST2.4.9 — Signal-chain FX order and performance sequencer

### Shared FX signal path

Each bassline has its own **Level** (shown *before Waveform*), Pan and
Distortion. The stereo Chorus follows distortion; its two independently
adjustable sends feed separate chorus lines with a shared modulation clock.
The **Delay Send 1/2** and **Reverb Send 1/2** then operate on the resulting
combined sound (distortion and chorus), rather than on oscillator dry alone.
**Fixed Delay → Reverb routing (TEST2.4.11):** delay echoes feed the
reverb together with the direct Reverb Sends. The alternate serial order
is no longer selectable; its historical VST ID is kept for old project files.

The downstream FX receives the upstream wet signal *in addition to* its own
per-bassline sends; the destination send pair scales this crossfeed, so both
sends at 0% prevent newly feeding that destination. One shared Classic or Dual
Delay and one shared selected Reverb are used; there are no duplicate delay
processors. Reverb tails continue after input is removed. The existing **Dry
Level** only controls the direct distorted bassline bus, not the wet returns.
The external REAPER audio input is not routed through the instrument FX.

This is a new routing scheme; existing `.lb` presets keep their numeric FX
controls but will sound different from the older *parallel* routing. The
reverb's intentionally high return gain from TEST2.4.7 remains, so watch
headroom, especially when delay repeats feed the reverb.

### Sequencer performance controls

The Sequencer Grid now includes **10 additional automatable controls per
bassline**, grouped beside its Sequencer On/Off, Pattern and MIDI Channel:

- **BPM Division**: 0.03125–64 (default **4.00**, as in LJuno), so a value
  of 4 means four steps per quarter-note at REAPER's tempo.
- **Shuffle**: 0–1; delays the alternate steps, preserving the BPM grid.
- **Legato**: Off/On; length-100 notes may overlap for glide.
- **Note Skip Probability**: 0–1 (una nuova decisione a ogni occorrenza dello step).
- **Note Random Depth**, **Velocity Random Depth**, **Length Random Depth**,
  **Repeat Random Depth**: 0–1, con variazioni a ogni passaggio musicalmente ripetuto
  (nessuna allocazione audio durante il calcolo casuale).
- **Global Octave Shift**: -4 to +4 octaves; **Global Semitone Shift**: -12 to +12.

**Aggiornamento TEST2.4.22:** questi dieci controlli ora appartengono al
**singolo pattern** della rispettiva bassline. Cambiando Pattern P1–P16 il
Sequencer Grid ricarica i valori di quel pattern. Lo stato precedente condiviso
e stato migrato in tutti i 16 pattern per non alterare i vecchi preset. La
Song usa i BPM Division individuali per calcolare la durata degli elementi.

### Build and verification

Source-only delivery. Run GitHub Actions for Windows VST3 (no local build).
Check the fixed Delay -> Reverb routing with chorus, verify FX Send = 0 edge
cases, then test BPM Division 4 / 8 / 2, Shuffle, and separate bassline
performance settings. Finally save and reload a `.lb` preset to verify the
new append-only automation parameter IDs and the existing 32 pattern banks.


## TEST2.4.15 — Copia/taglia/incolla un SOLO pattern (correzione TEST2.4.14)

Nell'editor Alt+Q, **Ctrl+Shift+C / Ctrl+Shift+X / Ctrl+Shift+V** ora
copiano, tagliano e incollano soltanto il **pattern selezionato** della
traccia corrente, non tutti i 16 pattern della bassline. Copia completa
di 128 step, Start/End, Launch Step, impostazioni del pattern e Parameter
Locks. Non sono inclusi parametri del sintetizzatore, FX, canali MIDI
VST3 o gli altri 15 pattern.

Puoi selezionare Bassline 1, Pattern 3, copiarlo con Ctrl+Shift+C,
poi cambiare su Bassline 2 (End), scegliere Pattern 8 (Ctrl+Su/Giù) e
incollarlo con Ctrl+Shift+V. Gli altri pattern restano invariati.
L'incolla sovrascrive il solo pattern di destinazione. Ctrl+Shift+X
azzera solo il pattern corrente. Ctrl+Z e Ctrl+Shift+Z annullano e
ripristinano soltanto l'operazione su quel pattern.

Gli appunti di Windows permettono il trasferimento fra istanze. Il
formato cambia da `LB303-TRACK-V1` a `LB303-PATTERN-V1`: le copie
prodotte dalla TEST2.4.14 sono rifiutate esplicitamente per evitare
che vengano incollate 16 sequenze per errore.

Ctrl+C/X/V resta invariato per i singoli step. I 32 pattern, i preset
`.lb` e gli ID delle automazioni non cambiano. Test statici soltanto:
la compilazione va verificata con GitHub Actions e REAPER.


## TEST2.4.17 — Arresto dell'anteprima alla chiusura della finestra VST3

Con Alt+P su Sequencer Preview, **Spazio** continua ad avviare o fermare
l'anteprima contemporanea delle due bassline in Grid/Value/Edit, nel browser preset
e nell'editor Alt+Q. **Esc o Alt+Q chiudono soltanto l'editor degli step**,
permettendo all'anteprima di proseguire mentre si regolano i parametri.

**Quando si chiude la finestra principale del VST3**, l'anteprima viene
sempre arrestata automaticamente, anche se l'istanza rimane caricata
in REAPER. Riaprire la finestra non riavvia la sequenza: occorre premere
nuovamente Spazio. La scelta Alt+P resta per istanza e non viene salvata
nei preset o nei progetti. I trigger MIDI e il normale trasporto REAPER
non vengono toccati.

Verifica statica superata; compilazione da effettuare con GitHub Actions
e confermare sul campo in REAPER/NVDA.


## TEST2.4.18 — Four stereo output buses

Outputs have independent roles: **Master 1/2** = complete LB-303 mix, **Bassline 1 3/4** = distorted Bassline 1 dry stem (when Separate Output enabled), **Bassline 2 5/6** = distorted Bassline 2 dry stem (when enabled), **FX Wet 7/8** = chorus + delay + reverb only. Existing per-bassline output parameters remain unchanged and default to Off. The full master includes both direct voices and a single shared FX return even when stem outputs are enabled. The stems are parallel taps, not additional dry signals written into the master. Master FX Dry Level controls only the main direct signal and does not attenuate stems; FX wet 7/8 contains no dry signal.

REAPER setup: increase track channel count to eight and open the VST3 pin connector to assign four distinct stereo output pairs. **Do not listen to Master 1/2 and the stem tracks summed together.** Master already includes those sounds. If you want to mix basslines and FX independently, monitor stems 3/4 + 5/6 + wet 7/8, and mute the master monitor. This is an unavoidable distinction between the requested full-mix master and isolated stems. No existing VST3 parameter ID, preset format, sequence, FX algorithm or keyboard shortcut is changed in this update.


### TEST2.4.20 — Ctrl+A selects the focused 16-step block

In Alt+Q, **Ctrl+A** selects only the 16 steps mapped to the computer
keyboard on the current page (for example 33–48), not all 128 steps.
Any older selection is replaced. If Start/End intersects only part of the
block, only its active steps are selected; NVDA announces the count and
first/last step. Ctrl+C/X/V still operate on selected individual steps,
Ctrl+Shift+C/X/V on the current pattern. The shortcut changes no preset data.

## TEST2.4.21 — Song Editor dentro Alt+Q

La nuova modalita Song organizza i 16 pattern per ciascuna bassline in due colonne
verticali **indipendenti e di lunghezza variabile**, senza limite fisso in numero
di righe (la lunghezza effettiva dipende dalla memoria e dai limiti dei file preset).
Ogni riga e un riferimento P1–P16: non copia gli step del pattern originale.
Le colonne hanno cursori ricordati separatamente, e non sono righe di una
singola timeline sincronizzata: i pattern possono avere Start/End differenti.

- Dentro Alt+Q, **Alt+S** alterna editor Steps e Song. Quando si entra in Song
  viene attivato Song playback; l'editor Steps mantiene tutte le scorciatoie
  e le 7 pagine preesistenti. Fuori da Alt+Q Alt+S salva i preset come prima.
- **Alt+M** ovunque in Alt+Q alterna Song preview e Pattern preview. **Alt+R** in Song continua a essere un comando alternativo. La modalita si
  salva in `.lb` e nello stato REAPER; Alt+P continua a decidere se Space
  controlla REAPER o l'anteprima, senza modificare il tipo di riproduzione.
- **Alt+Shift+M** ascolta soltanto il P1–P16 della riga focalizzata tramite l'unico
  motore anteprima; un'altra pressione sullo stesso elemento lo ferma.
- Frecce sinistra/destra cambiano bassline e ripristinano la posizione precedente
  dell'altra colonna; su/giu avanza di una riga, Pagina su/giu di **5**;
  Home/End va alla prima/ultima riga. I bordi sono silenziosi.
- **Tab** pronuncia Bassline, riga, numero totale, Pattern, Loop, modalita
  playback e numero selezionato. Le frecce pronunciano soltanto, per esempio,
  `P3, 24 of 64`; `Selected` e pronunciato sulle righe selezionate soltanto
  se la selezione multipla contiene almeno due elementi.
- **Ctrl+Su/Giu** modifica immediatamente il riferimento P1–P16 della riga
  corrente, proprio come la scelta pattern in Steps. **Invio** legge i dettagli,
  senza aprire un selettore. **Alt+I** inserisce dopo il cursore un'altra
  occorrenza del pattern corrente; se la colonna e vuota inserisce P1.
- **Alt+L** cambia il Loop solo della bassline corrente. Con Loop Off una
  colonna si ferma alla fine, mentre l'altra puo continuare. Con Loop On la
  colonna ricomincia indipendentemente; entrambe condividono solo il BPM.
- **Shift+su/giu** amplia o restringe un intervallo. Rilasciando Shift,
  frecce normali spostano il cursore senza cancellare la selezione.
  **Shift+Spazio** oppure **Ctrl+Spazio** seleziona/deseleziona esplicitamente la riga focalizzata; **Ctrl+Shift+su/giu**
  consente di aggiungere un altro intervallo disgiunto; **Ctrl+A** seleziona
  tutte le righe della sola colonna corrente.
- **Ctrl+C/X/V** copia/taglia e incolla le righe selezionate, in ordine,
  *senza includere gli elementi deselezionati tra due gruppi*. Incolla DOPO
  l'elemento focalizzato senza sovrascrivere le righe esistenti. Gli appunti
  sono condivisibili fra istanze LB-303, con formato `LB303-SONG-ITEMS-V1`.
- **Delete** cancella uno o piu elementi; **Ctrl+Delete** chiede conferma
  per svuotare la colonna. **Ctrl+Z / Ctrl+Shift+Z** ripristina un'intera
  operazione, anche quando comprende molti elementi.

Le colonne, il loop e la modalita Song/Pattern sono salvati nei preset `.lb` e
nei progetti REAPER mediante un nodo XML aggiuntivo; i preset piu vecchi
restano compatibili e caricano una Song vuota in Pattern mode. Non si salva
la posizione dei cursori, ne le selezioni o gli appunti. La versione mantiene
tutti i parametri VST3, i 32 pattern, gli FX, gli output a 8 canali, i due
oscillatori per bassline e i due LFO.

**Verifica richiesta**: compilazione su GitHub Actions e prova pratica REAPER/NVDA.
I test inclusi sono statici e non dimostrano che audio, routing, lock e screen
reader funzionino correttamente a runtime. In particolare verificare i cambi
fra pattern di lunghezza diversa, i loop indipendenti e Alt+M mentre l'anteprima
Song e gia in riproduzione.


## TEST2.4.35 – True idle sleep for silent voices

With no held notes or audible tails, neither synth oscillator/filter nor the
2x/4x distortion/downsampling graph is called. The sequencer/MIDI/timing
code remains active and wakes the voice on the exact incoming note offset.
A voice enters sleep only after its gate, release envelope, resonant filter
state, output biquads and oversampling filters have decayed below a
conservative numerical threshold. Effects still process wet tails, with a
delay-time/feedback and reverb-decay-aware safety guard extended while
the wet return is measurable;
the plugin does **not** assume that silence between long delay repeats means
that the delay can be turned off. No new controls or plugin parameters.

REAPER idle CPU includes host overhead and can vary by audio buffer size,
sample rate, FX settings, and transport mode; no fixed percentage is promised.

Unchanged parameter arrays and tempo no longer rebuild filters and nonlinear
DSP coefficients every processBlock call; changing an actual parameter or
oversampling quality still refreshes the coefficients.

MIDI CC123 All Notes Off (and CC120 All Sound Off) now release the gate
without abruptly truncating the voice filter decay. This prevents a stuck
voice from keeping the oversampled engine awake after transport-stop MIDI.


### TEST2.4.37 - Browser: ricorda soltanto un caricamento confermato
- Alt+B si posiziona esclusivamente sull'ultimo preset realmente caricato con Invio o con i comandi di caricamento.
- Scorrere/anteprima e uscire con Escape non cambia la posizione ricordata, anche riaprendo subito il browser nella stessa istanza.
- Salvare un preset non cambia il riferimento all'ultimo preset caricato.
- La preferenza rimane persistente anche tra istanze, progetti e riavvii.
- L'eventuale cartella scelta per Salva con nome è distinta e non influenza il browser di caricamento.


### TEST2.4.52 — Portamento Curve (per Bassline)

In Bassline 1 e Bassline 2, vicino a Portamento Time e Portamento Mode, il nuovo selettore **Portamento Curve** consente: **Original** (default, esattamente il portamento storico), **Linear** (uniforme in semitoni), **Exponential** (partenza veloce, rallenta in arrivo), **Logarithmic** (partenza lenta, accelera), **S-Curve** (morbida all’inizio e alla fine). Le curve alternative conservano il tempo di arrivo al 99% del portamento originale e funzionano con Always, Legato e Length 101. Ogni bassline ha il suo selettore indipendente; 0/Original non modifica alcun preset già salvato. Gli ID VST3 precedenti e i Parameter Locks restano invariati.

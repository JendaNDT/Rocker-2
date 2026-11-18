# Rocker 2 — funkce a ovládání

**Dokument 01 z 5.** Návrh domácího nahrávacího studia pro Windows očima muzikanta. Dokument je samostatný a nepředpokládá znalost předchozího rozhovoru.

- **Pro koho je studio:** Jendův kamarád hraje na elektrickou kytaru a na bicí. Doma chce zachytit riff, postupně přidávat nástroje, skládat z nahraných částí písničky a míchat je.
- **Kdo dokument čte:** Jenda (není programátor) a model, který bude aplikaci programovat (Astra nebo Sol). Technické podrobnosti jsou v dokumentu 02.
- **Související dokumenty:** 02_ARCHITEKTURA.md (technologie a tok zvuku), 03_ZKOUSKY_NA_WINDOWS.md (jak ověříme, že funguje), 04_ETAPY_VYVOJE.md (pořadí prací), 05_STARTOVACI_PROMPT.md (zadání první etapy).

### Značky důvěryhodnosti

U tvrzení o původním Rockeru i u návrhů rozlišuji tři druhy podkladu:

| Značka | Význam |
|---|---|
| **[K]** | Doloženo v kódu původního Rockeru (odkaz na konkrétní soubor a řádky). |
| **[N]** | Vlastní návrh pro Rocker 2. V původní aplikaci tato věc není, nebo tam funguje jinak. |
| **[Z]** | Musí se vyzkoušet na skutečném nástroji a počítači. Papír ani automatický test to nerozhodne. |

Odkazy na původní repozitář míří na commit `77df8e3` ze 3. 6. 2026 (poslední commit v době průzkumu). Starý repozitář jsem jen četl, nic jsem v něm neměnil.

> **Pravidlo pro vzhled.** Vzhled a rozložení Rockeru 2 (kapitola 5) jsou navržené od nuly. Ze starého repozitáře jsem studoval jen hudební logiku, tedy zvuk, časování, ukládání a datové modely. Snímky obrazovek, styly, barevná témata ani vizuální komponenty jsem neotevíral a nepoužil jsem je jako předlohu. Stejné pravidlo platí pro implementaci.

---

## 0. Rocker 2 v jedné větě

**Rocker 2 je jedno okno, ve kterém kytarista a bubeník zachytí nápad během pár vteřin, postaví na něm skladbu stopu po stopě, opraví jediný takt bez přehrávání celé písně, smíchá ji a pošle kamarádovi. Nic přitom neztratí, ani když program spadne.**

Tři vlastnosti, na kterých stojí všechno ostatní:

1. **Stopy k sobě sedí.** Kytara nahraná přes bicí sedí na úder i po pěti minutách. Starý Rocker posun způsobený zpožděním zvukovky vědomě neřešil.
2. **Zvuk kytary jde změnit i po nahrání.** Nahrává se čistý signál z kytary a aparát je nastavení stopy, ne součást nahrávky.
3. **Nic se neztratí.** Nahrávka se zapisuje na disk průběžně a skladba se ukládá sama. Po pádu nebo výpadku proudu se otevře poslední zachovaný stav.

---

## 1. Pro koho a v jaké situaci

**Muzikant:** hraje na elektrickou kytaru a bicí. Nahrává doma, sám, na běžný notebook s Windows. Sluchátka zapojí do zvukové karty. Model zvukové karty ani zapojení bicích zatím neznáme. Rocker 2 proto počítá se všemi běžnými sestavami a tu skutečnou pozná sám (kapitola 7).

**Rocker 2 se používá ze tří vzdáleností:**

| Situace | Kde muzikant je | Co potřebuje | Důsledek pro návrh |
|---|---|---|---|
| **Za bicími** | 2–3 m od notebooku, paličky v rukou | Z dálky poznat, že záznam běží, kolik zbývá do odpočtu a v jakém je taktu | Celé okno při nahrávání zčervená a existuje „Pohled zdálky“ s obřími čísly (5.6) |
| **S kytarou v ruce** | 1 m od notebooku, obě ruce na nástroji | Spustit a zastavit záznam bez myši a slyšet zvuk aparátu bez zpoždění | Ovládání jednou velkou klávesou, nožním spínačem nebo numerickou klávesnicí (5.10) |
| **U klávesnice** | Sedí u notebooku, střih a mix | Přesně vybrat začátek taktu a porovnat pokusy | Velká klikací čísla taktů, přichytávání na mřížku a lupa při tažení (5.4) |

---

## 2. Dvě cesty jednou skladbou

### 2.1 Od rychlého nápadu k rozpracované skladbě

> Kamarád sedí na gauči s kytarou a přijde mu do prstů riff.

1. **Zachytí ho.** Na úvodní obrazovce stiskne **R** (nebo nožní spínač či velké tlačítko **● Nahrát nápad**). Záznam začne okamžitě, bez odpočtu a bez nastavování. Rocker 2 si navíc pamatuje **2 sekundy před stiskem**, takže se neusekne první úder, který zahrál, zatímco sahal po klávese. [N]
2. **Nestihl to?** Zahrál riff ještě předtím, než stiskl Nahrát? Stiskne **U – Ulož, co jsem hrál**. Rocker 2 má zapnutou **Pojistku**: když je otevřený a vstup aktivní, drží v paměti poslední 2 minuty z kytary. Tlačítko je uloží jako nápad. [N]
3. **Nápad se uloží sám**, například jako „Nápad 18. 11. 21:40“. Nikdo se ho neptá na název, dokud nechce. Hned ho může přehrát, označit hvězdičkou **★ Povedené** a připsat poznámku („Drop D, svižně, sloka?“).
4. **Druhý den** otevře knihovnu **Nápady**, vyfiltruje si jen ★ a poslechne si je (každý nápad má náhled průběhu zvuku, délku, datum a ladění).
5. **Udělá z nápadu skladbu.** Zvolí **Udělat z toho skladbu**. Vznikne skladba, ve které je nápad první stopou „Kytara“.
   - Pokud nahrával s klikem, tempo a takty už sedí.
   - Pokud hrál volně, Rocker 2 nabídne **Doťukat tempo**: pustí nápad a kamarád do něj 8× ťukne mezerníkem na doby. Rocker 2 z toho spočítá tempo i první dobu a nasadí mřížku taktů tak, aby seděla na nahrávku. [N] Riff hraný bez kliku se může mírně zrychlovat. Mřížka proto sedí na začátku a dál jen přibližně, což Rocker 2 řekne rovnou: „Na volně hraný nápad mřížka sedí přibližně. Finální verzi nahraj s klikem.“
6. Teď už pokračuje jako ve 2.2: nahraje k riffu bicí, přidá další kytaru a podobně.

**Proč takhle:** nápad nesmí čekat na založení projektu, výběr složky ani nastavení tempa. Strukturu (tempo, takty, díly) lze doplnit kdykoli později a nic se tím neztratí.

### 2.2 Skladba, která začíná bicími

U každého kroku je popsáno, co muzikant udělá, co uslyší a uvidí a jak sám pozná, že vše funguje.

#### Krok 1 — připojit nástroj, naladit, zkontrolovat signál

| | |
|---|---|
| **Udělá** | Zapojí kytaru do vstupu zvukovky označeného INST nebo Hi-Z a mikrofon (nebo výstup elektronických bicích) do dalších vstupů. Ve skladbě vytvoří stopy **Bicí** a **Kytara** a každé přiřadí vstup. Stiskne **T** a naladí kytaru. |
| **Uslyší** | Ve sluchátkách přesně to, co ukazuje panel **Co slyším**, například: „Kytara živě se zvukem Crunch · Klik · nic dalšího“. |
| **Uvidí** | V panelu **Co slyším / Co nahrávám** (spodní deska v režimu Hraj): <br>• **Ve sluchátkách:** seznam zdrojů a u každého malý měřič <br>• **Nahraje se:** „Vstup 1 → Kytara“, „Vstupy 3–4 → Bicí“ <br>• **Zpoždění ve sluchátkách:** například „7 ms – dobré“ <br>• měřič každého vstupu v dBFS s červenou značkou při přebuzení |
| **Ověří** | Brnkne na kytaru a rozsvítí se jen měřič vstupu 1. Udeří do virblu a rozsvítí se jen vstupy bicích. Když je vstup přebuzený, objeví se hláška „Kytara je přebuzená už na vstupu. Ubrat GAIN na zvukovce. Tohle se v mixu opravit nedá.“ Ladička ukazuje u všech strun zelenou a ±0 centů. |

#### Krok 2 — tempo, odpočet, osm taktů bicích, pak kytara

| | |
|---|---|
| **Udělá** | Nastaví tempo (napíše 112 nebo ho doťuká klávesou) a metrum 4/4. Odpočet nechá na 1 takt. Zapne nahrávání na stopě Bicí, stiskne **F11** (Pohled zdálky), sedne za bicí a stiskne **R** (nebo velký Enter na numerické klávesnici u bicích). |
| **Uslyší** | Čtyři doby odpočtu a potom klik. Svoje bicí slyší naživo z místnosti; ve sluchátkách je jen klik (u akustických bicích to tak je správně, viz 4.4). |
| **Uvidí** | Na celé obrazovce obří 4 – 3 – 2 – 1, pak celá plocha zčervená s nápisem **NAHRÁVÁM** a číslem taktu **5 / 8** (osm taktů si předem vybral smyčkou nebo jen sleduje číslo). |
| **Ověří** | Po zastavení se obrazovka změní na „Uloženo ✓ Bicí, pokus 1, 8 taktů“. |
| **Pak kytara** | Vypne nahrávání u bicích, zapne ho u kytary, stiskne **R**. Při dalším průchodu slyší ve sluchátkách nahrané bicí, klik a svou kytaru živě přes aparát Crunch. Nahraje se čistý signál a zvuk aparátu zůstává nastavením stopy. |
| **Zítra jiný zvuk** | Otevře zvuk stopy Kytara a přepne z Crunch na Lead nebo otočí Drive. Táž nahrávka zní hned jinak a nic se nepřehrává ani nepřepočítává. |
| **Ověří souhru** | Přehraje obě stopy. Kytara sedí na kopák i v posledním taktu. Pro kontrolu oddálí pohled: začátky úderů kytary a bicích leží na stejné svislé čáře mřížky. (Přesné měření je v dokumentu 03, zkouška M-02.) |

#### Krok 3 — chyba v jednom taktu

> V taktu 6 kytara ujela.

| | |
|---|---|
| **Udělá** | Klikne na číslo **6** na pravítku taktů (vybere se takt 6 na stopě Kytara) a stiskne **P – Oprav takt**. |
| **Uslyší** | Rocker 2 začne hrát dva takty předem (od taktu 4), aby se chytil. Nahrávat začne přesně na začátku taktu 6 a skončí na jeho konci. Hraje ještě jeden takt po opravě (takt 7), aby slyšel návaznost, a pak se zastaví. Před začátkem zazní odpočet, pokud ho má zapnutý. |
| **Uvidí** | Ve stavovém pásu „OPRAVA · takt 6 · pokus 2“. Nad taktem 6 se na stopě objeví štítek **Pokusy 1 | 2**. |
| **Porovná** | Rocker 2 hned přehraje takty 5–7 s novým pokusem. Klávesou **Tab** přepíná mezi pokusem 1 a 2 za běhu na stejném místě (nápad A/B porovnání ze starého Rockeru). |
| **Ponechá** | **Enter – Ponechat tenhle.** Druhý pokus zní, první se schová, ale nesmaže. Na jiný pokus se může kdykoli vrátit kliknutím na štítek. |
| **Další pokus** | Stačí znovu **P**. Vznikne pokus 3 a štítek ukáže **1 | 2 | 3**. |
| **Ověří** | Přechody do opraveného taktu a z něj jsou plynulé, bez cvaknutí. Rocker 2 na hranách automaticky prolne 10 ms. Protože se nahrává čistá kytara a aparát se přidává až při přehrávání, nezazní na hraně ani useknutý dozvuk zkreslení. [N] |

**Proč takhle jednoduše:** muzikant nemusí znát pojmy punch-in, comping ani crossfade. Vybere takt a stiskne jednu klávesu. Vše ostatní (předstih, konec, prolnutí, uložení pokusu) je výchozí chování.

#### Krok 4 — sloka, refrén a refrén podruhé

| | |
|---|---|
| **Udělá** | Na pásu **Díly** nad stopami rozdělí nahrávku na díly. Klikne mezi takty 8 a 9 a zvolí **Rozdělit díl zde**. Díly pojmenuje **Sloka** a **Refrén** a každý dostane barvu jako proužek lepicí pásky. Pak refrén chytí a s klávesou **Ctrl** ho přetáhne na konec skladby, nebo na něm stiskne **D – Zopakovat**. |
| **Uvidí** | Na pásu Díly: `Sloka · Refrén · Refrén ⧉`. Značka **⧉** znamená **spojený výskyt**: oba refrény jsou tentýž díl, jen se hraje dvakrát. |
| **Co to znamená** | Opraví-li takt v refrénu, změní se oba výskyty. Rocker 2 to před opravou řekne: „Refrén se hraje 2×. Oprava platí pro oba. **[Opravit oba]** **[Jen tento výskyt]**“. Volba *Jen tento výskyt* z druhého refrénu udělá samostatný díl „Refrén (2)“. Tomu se říká **osamostatnit**. |

**Když zkrátí jen druhý výskyt** (například chce, aby měl jen 6 taktů místo 8):

1. Chytí pravý okraj druhého refrénu na pásu Díly a táhne doleva. Okraj přeskakuje po celých taktech.
2. **Co se stane:** zkrátí se *jen tento výskyt*. První refrén má dál 8 taktů. Všechno za druhým refrénem se automaticky posune o 2 takty dřív, takže ve skladbě nevznikne díra. Na konci zkráceného výskytu se zvuk krátce prolne, aby necvaklo.
3. **Co uvidí:** druhý refrén má štítek „6 z 8 taktů“ a za okrajem šrafovaný zbytek, který připomíná, že materiál tam pořád je.
4. **Nic se nesmazalo.** Zkrácení je jen „schování“ konce. Nahraný zvuk zůstává na disku i v dílu Refrén.
5. **Jak změnu vrátí** (tři rovnocenné cesty):
   - **Ctrl+Z** (Zpět), hned nebo i po dalších úpravách,
   - přetáhne okraj zpět na plnou délku,
   - pravým tlačítkem na výskyt zvolí **Obnovit celou délku**.

> **Omezení první verze:** když výskyt dílu skončí uprostřed doznívajícího činelu, dozvuk se na hraně utne (s krátkým prolnutím). Volba „Nechat doznít přes hranu“ je v rozšířeních (kapitola 6). [N]

#### Krok 5 — mix, uložení, pokračování zítra, odeslání

| | |
|---|---|
| **Mix** | Stiskne **3** (režim Míchej). Každá stopa má ve svém řádku vodorovný jezdec **Hlasitost** (v dB), **Polohu** ve stereu (L … střed … P), tlačítka **Sólo** a **Ztlumit** a měřič. Kytaru dá trochu doleva, bicí nechá uprostřed a ubere bicí o 2 dB. |
| **Ověří** | Měřič **Celkem** (master) nesmí ukázat PŘEBUZENO. Kdyby ukázal, Rocker 2 nabídne „Ubrat celkovou hlasitost o 3 dB“. |
| **Uložení** | Nemusí dělat nic. Každá změna se do 1–2 sekund uloží sama a stavový pás ukazuje „Uloženo ✓“. **Ctrl+S** jen potvrdí uložení. |
| **Zítra** | Otevře Rocker 2. Úvodní obrazovka nabídne **Pokračovat: Noční riff (včera 23:12)**. Skladba se otevře se stejným režimem, přiblížením, vybranou stopou a ukazatelem na stejném taktu. Zvuk aparátu i mix jsou stejné. |
| **Odeslání** | **Ctrl+E – Poslat mix.** Zvolí MP3 (k poslechu v telefonu) nebo WAV (nejvyšší kvalita). Rocker 2 vytvoří soubor „Noční riff – mix 2026-11-19.mp3“, otevře složku s vybraným souborem a kamarád ho přetáhne do zprávy nebo e-mailu. |
| **Ověří** | Klikne na **Přehrát výsledný soubor** a poslechne si přímo vytvořený soubor. Mix zní stejně jako při míchání, bez kliku. Je-li zapnuté sólo, Rocker 2 před exportem upozorní: „Sólo je zapnuté na stopě Kytara. Mix by obsahoval jen ji. **[Exportovat vše]** **[Jen sólo]**“. |

---

## 3. Mapa hudebních funkcí původního Rockeru

**Co jsem procházel:** zdrojový kód zvuku (`src/audio/`), logiku (`src/lib/`), stav (`src/stores/`) a funkční části komponent (ne vzhled), plus projektovou dokumentaci (`ARCHITECTURE.md`, `PROJECT_STATUS.md`, `ROADMAP.md`).

**Celkový obraz:** Rocker je webová aplikace (PWA) v Next.js a zvuk zpracovává přes Web Audio API v prohlížeči. Je to hlavně **cvičný asistent** (lekce, trenažéry, AI tutor). Nahrávání a studio jsou v něm doplňky. Projektová dokumentace sama uvádí, že aparát, studio a smyčkový pedál byly „postavené naslepo“ a zvuk, latence ani vrstvení nebyly ověřené s kytarou ([PROJECT_STATUS.md ř. 44–50][st-naslepo], [ROADMAP.md ř. 217–234][rm-naslepo]).

**Jak číst tabulku:** sloupec *Rozhodnutí* má hodnoty **Převzít** (nápad zůstává, uvnitř se řeší nově), **Upravit** (nápad zůstává, ale chová se jinak) a **Vynechat**. Priorita **v1** znamená první ucelenou verzi, **v1.1** první rozšíření a **v2** pozdější rozšíření.

### 3.1 Ladička a ladění

| Funkce Rockeru (doklad) | Konkrétní použití při tvorbě písně | Rozhodnutí | Priorita a důvod |
|---|---|---|---|
| Ladička s detekcí výšky (McLeod/Pitchy), práh jistoty 0,9, vyhlazení ručky a podržení posledního tónu 600 ms. [K] [useTuner.ts ř. 11–16][tuner-hook] | Naladit před nahráváním a zkontrolovat mezi pokusy, protože po hraní se struny rozlaďují. | **Převzít** (nově naprogramovat v jádru) | **v1, vysoká.** Rozladěnou stopu nejde opravit. [Z] Ověřit stabilitu na nejhlubší struně Drop B (B1 ≈ 61,7 Hz). |
| Vedená ladička: ±5 centů = naladěno, potvrzení po 400 ms ustáleného tónu. [K] [GuidedTuner.tsx ř. 16–20][guided] | Ladí bez dívání: struna po struně a potvrzení zazní, až je struna v toleranci. | **Upravit:** potvrzovací zvuk jde jen do sluchátek a nikdy do nahrávky. | **v1.** |
| 9 továrních ladění (Standard, Drop D/C/B, Eb, D std, Open G/D, DADGAD). [K] [tunings.ts ř. 20–129][tunings] Vlastní ladění. [K] [customTunings.ts][custom-tunings] | Píseň v Drop D a jiná ve standardu. | **Převzít a rozšířit:** **ladění se pamatuje ve skladbě.** Při otevření Rocker 2 připomene „Tato skladba je v Drop D“. [N] | **v1.** Zapomenuté přeladění je častá chyba při návratu ke skladbě. |
| Referenční tón struny (sinus s 2. a 3. harmonickou, podržením hraje). [K] [referenceTone.ts ř. 1–20][reftone] | Naladit podle sluchu nebo zkontrolovat oktávu. | **Převzít.** | **v1, nízká.** Levné a občas užitečné. |
| Šumová brána ladičky s automatickou kalibrací („změř ticho“). [K] [pitchDetector.ts ř. 34–51][gate] | Hučící snímače a ruch v místnosti. | **Upravit:** princip „Změř ticho“ se použije pro bránu v aparátu a později pro funkci „Spustit hraním“. | **v1.** |
| Komorní A pevně 440 Hz. [K] [pitch.ts ř. 3][a440] | Hraní k podkladu laděnému jinak (např. 442 Hz). | **Upravit:** nastavitelné 430–450 Hz. | **v1.1, nízká.** |

### 3.2 Tempo, metronom a odpočet

| Funkce Rockeru (doklad) | Konkrétní použití | Rozhodnutí | Priorita a důvod |
|---|---|---|---|
| Metronom s plánováním přesným na vzorek („dvoje hodiny“), tempo 40–240. [K] [metronome.ts ř. 1–9][metro], [tempo.ts ř. 3–4][tempo] | Tempo skladby, mřížka taktů, klik do sluchátek. | **Převzít princip.** Klik vzniká přímo ve zvukovém jádru, na stejných hodinách jako nahrávání. | **v1, vysoká.** Bez přesného kliku nesedí takty. |
| 7 taktových měr (2/4 … 12/8) a přízvuky dob. [K] [metronomeConfig.ts ř. 100–113][signatures] | Metrum skladby. | **Převzít.** | **v1.** |
| Odpočet 0–2 takty. [K] [metronomeConfig.ts ř. 164][countin-max], [countIn.ts][countin] | Před nahráním i před opravou taktu. | **Převzít a rozšířit:** odpočet i pro opravu taktu a obří odpočet v Pohledu zdálky. [N] | **v1, vysoká.** |
| Ťukání tempa (průměr intervalů). [K] [tempo.ts ř. 13–28][tap] | Tempo riffu, který má v hlavě. | **Převzít a rozšířit** o „Doťukat tempo“ k nahranému nápadu (2.1). [N] | **v1.** Spojuje rychlý nápad a skladbu. |
| 5 zvuků kliku, hlasitost, hlasitost přízvuku. [K] [metronomeConfig.ts ř. 173][clicks] | Slyšet klik přes hlasité bicí. | **Upravit:** 2–3 průrazné zvuky, samostatná hlasitost kliku, **klik jde jen do sluchátek**. | **v1.** |
| „Drum mode“ (kopák, virbl, hi-hat podle dob). [K] [metronome.ts ř. 74–80][metro-state] | Kytarista nahrává dřív než bicí a místo suchého kliku chce jednoduchý rytmus. | **Upravit** na „Vodicí rytmus“ (jen do sluchátek). | **v1.1, střední.** |
| Swing/shuffle 50–75 %. [K] [metronomeConfig.ts ř. 120–123][swing] | Shuffle skladby. | **Převzít později.** | **v1.1, nízká.** |
| Trenažér zrychlování, cvičné tiché takty, polyrytmus, kyvadlo, tovární předvolby metronomu. [K] [metronomeConfig.ts ř. 250–306][trainers], [metronomePresets.ts ř. 73][metro-presets] | Cvičení, ne tvorba. | **Vynechat.** | Při skládání nepomáhají a zvětšují ovládání. |

### 3.3 Aparát a efekty

| Funkce Rockeru (doklad) | Konkrétní použití | Rozhodnutí | Priorita a důvod |
|---|---|---|---|
| Řetězec: komprese → předzesílení → zkreslení (3 tvary) → 3pásmový EQ → simulace reproboxu filtry → delay, dozvuk, chorus. [K] [ampChain.ts ř. 1–13][amp-chain], [ř. 38–84][amp-curves] | Kytara zní při nahrávání jako přes aparát. | **Upravit zásadně:** **nahrává se čistý signál** a aparát je *zvuk stopy*, který jde kdykoli změnit. Zkreslení s převzorkováním (méně „písku“), reprobox z impulsní odezvy, deterministický dozvuk. [N] | **v1, vysoká.** Zadání chce „zítra jiný zvuk na téže nahrávce“. |
| Nahrávání „zaaparátovaného“ zvuku (záznam za efekty). [K] [ampEngine.ts ř. 1–11][amp-engine], [ampChain.ts ř. 122–123][amp-recstream] | Zvuk je napevno zapečený v nahrávce. | **Vynechat jako výchozí.** Nahrazuje ho čistý záznam a volitelné „Zmrazit zvuk“ (v1.1). | Zapečený zvuk nejde zítra změnit. |
| 4 předvolby: Čistý, Crunch, Lead, Metal a vlastní předvolby. [K] [ampPresets.ts ř. 41–62][amp-presets] | Rychlý start zvuku. | **Převzít** (názvy jsou obecné a dobré). **Předvolba se do skladby kopíruje**, takže skladba zní stejně, i když se knihovna předvoleb změní. [N] | **v1.** |
| Dozvuk z náhodného šumu generovaný při každém zapnutí. [K] [ampChain.ts ř. 86–102][amp-reverb] | — | **Upravit:** dozvuk musí být při každém přehrání i exportu stejný. | **v1.** Jinak by se export lišil od poslechu. |
| Aparát hraje dál při přepínání obrazovek (sdílený engine). [K] [ampEngine.ts ř. 1–11][amp-engine] | Zvuk se nepřeruší při přechodu ze střihu do mixu. | **Převzít jako zásadu:** jedno okno a zvuk běží nepřetržitě. | **v1.** |
| Zvuk ani latence aparátu nebyly v Rockeru ověřeny. [K] [ROADMAP.md ř. 217–222][rm-naslepo] | — | — | [Z] V Rockeru 2 je ověření zvuku s kytarou podmínkou dokončení etapy (04). |

### 3.4 Nahrávky, porovnání, smyčka, studio, export

| Funkce Rockeru (doklad) | Konkrétní použití | Rozhodnutí | Priorita a důvod |
|---|---|---|---|
| **Live režim:** ladička, tempo a jedno tlačítko Rec na jedné obrazovce, obrazovka nezhasíná. [K] [live/page.tsx ř. 23–34][live] | Zachytit nápad s kytarou v ruce. | **Převzít nápad** jako **Nahrát nápad** s 2 s předstihem a Pojistkou. Windows během nahrávání neusne. [N] | **v1, vysoká.** |
| Nahrávky: seznam, přejmenování, **značky míst** v nahrávce, stažení souboru. [K] [recordings.ts ř. 5–25][rec-model], [RecordingItem.tsx ř. 97–122][rec-item] | Najít povedený pokus později. | **Převzít a rozšířit** na knihovnu Nápady: ★, poznámka, tempo, ladění, náhled průběhu. Značky se ve skladbě stávají díly. | **v1.** |
| **A/B porovnání** dvou nahrávek, při přepnutí se drží stejné místo. [K] [AbCompare.tsx ř. 10–14, 109–120][ab] | Vybrat lepší pokus. | **Převzít** jako přepínač pokusů (**Tab**) přímo v opravovaném taktu. | **v1, vysoká.** |
| **Smyčkový pedál:** základní smyčka a vrstvy přes ni. Vrstvy se slévají do jednoho zvuku (nejde vrátit jednu vrstvu), délka není svázaná s tempem, vrstva se posouvá o zpoždění systému a smyčka **nejde uložit ani přenést do studia**. [K] [loopPedal.ts ř. 1–8, 154–172][loop], [loopMix.ts][loopmix], [LoopPedal.tsx][loop-ui] | Hledání riffu: hraje dokola a zkouší, co sedí. | **Upravit:** smyčka je **vybraný úsek taktů přímo ve skladbě**. Každý průchod nahraje nový pokus nebo vrstvu na stopu. Vrstvy zůstávají oddělené (jde je ztlumit, smazat, změnit jim zvuk) a jsou srovnané s takty. **Povedená smyčka je tím pádem rovnou ve skladbě**: „Udělat z ní díl“ a pak díl opakovat. Upravit ji jde kdykoli. [N] | **v1.1, střední.** V1 má přehrávání ve smyčce a pokusy, což pro opravy stačí. Nahrávání dokola je první rozšíření. |
| **Vícestopé studio:** nahrávání přes stopy, hlasitost, ztlumení, smazání. Posun o latenci se **nekompenzuje** a projekt **žije jen v paměti**. [K] [multitrackEngine.ts ř. 1–8][mt-latency], [Studio.tsx ř. 35–37][studio-memory], [multitrack.ts ř. 7–22][mt-model] | Základ nového studia. | **Upravit od základu:** kompenzace zpoždění, časová osa s takty, poloha ve stereu, sólo, automatické ukládání, pokusy, díly. | **v1, nejvyšší.** Bez toho nevznikne skladba. |
| Export mixu do 16bit WAV. [K] [multitrackEngine.ts ř. 133–169][mt-mixdown], [wav.ts ř. 12][wav] | Poslat mix kamarádovi. | **Upravit:** WAV 24 bit i MP3, kontrola přebuzení a export odpovídající tomu, co hráč slyšel. | **v1.** |
| Sečtení dvou vstupů zvukovky do mona (kytara hrála jen vlevo). [K] [audioEngine.ts ř. 36–44][input-mono], [PROJECT_STATUS.md ř. 519–529][st-mono] | Kytara musí znít v obou sluchátkách. | **Upravit:** u každé stopy výslovná volba vstupu (*Vstup 1*, *Vstup 2*, *Vstupy 1+2 stereo*). Mono stopa zní uprostřed. | **v1.** Odstraní celou třídu záhad. |
| Měřič vstupu s indikací přebuzení. Ve skutečnosti měří RMS × 3, ne špičky vzorků. [K] [InputLevelMeter.tsx ř. 9–10, 48–60][meter] | Nastavení gainu na zvukovce. | **Upravit:** skutečné špičky v dBFS. Přebuzení vstupu se odlišuje od přebuzení mixu. | **v1, vysoká.** |
| Odhad zpoždění jen z výstupní části (base + output latency). [K] [audioEngine.ts ř. 330–335][latency-est] | Kolik zpoždění cítí v prstech. | **Upravit:** měřit celou cestu tam i zpět, s kalibrací kabelem. | **v1.** |
| Záloha nastavení do JSON (**bez nahrávek**). [K] [backup.ts ř. 1–5][backup] | Přenos na jiný počítač. | **Upravit:** **Zabalit skladbu** do jednoho souboru včetně všeho zvuku. | **v1.** |
| Ochranné načítání uložených dat (poškozený záznam se zahodí, aplikace nespadne). [K] [multitrack.ts ř. 115–140][mt-guard], [ampPresets.ts ř. 124–151][amp-sanitize] | Starý nebo poškozený soubor skladby. | **Převzít jako zásadu:** poškozenou část skladby zahodit s hlášením, zbytek otevřít. | **v1.** |

### 3.5 Funkce pro cvičení a učení (mimo tvorbu)

| Funkce Rockeru (doklad) | Rozhodnutí a důvod |
|---|---|
| Riff trainer, YouTube přehrávač se smyčkou a zpomalením (`preservesPitch`). [K] [RiffTrainer.tsx ř. 62–63][slowdown] | **Vynechat.** Slouží k učení cizích skladeb. Pozdější nápad: *zpomalit vlastní nahrávku pro nácvik sóla* (v2). |
| Noty a taby (AlphaTab, Guitar Pro). [K] | **Vynechat.** Pro nahrávání vlastní hudby nejsou potřeba. |
| Lekce, AI tutor (Google Gemini, vyžaduje internet a klíč), hry, metalové trenažéry, denní plán, achievementy, připomínka přestávky. [K] [README][readme] | **Vynechat.** Nesouvisí s tvorbou a AI vyžaduje internet, kdežto Rocker 2 má fungovat bez účtu i připojení. |
| Profil nástroje a tónový poradce (doporučí gain, EQ a snímač podle stylu). [K] [instrument.ts ř. 1, 201][instrument] | **Vynechat** z v1. Nápovědy u předvoleb jsou v2. |
| Tři vizuální témata, PWA a offline service worker. [K] | **Vynechat.** Vzhled je nový podle zadání a nativní aplikace funguje offline z podstaty. |

### 3.6 Tři nápady, které přenesu jako první

1. **Metronom s odpočtem a ťukáním tempa** se stane páteří skladby: mřížka taktů, klik jen do sluchátek, odpočet i před opravou taktu.
2. **Aparát s předvolbami Čistý, Crunch, Lead a Metal** se stane *zvukem stopy*, který jde měnit po nahrání.
3. **A/B porovnání na stejném místě** se stane přepínačem pokusů (Tab) při opravě jednoho taktu.

Hned za nimi jde **Live režim**, ze kterého vzniká „Nahrát nápad“.

---

## 4. Rocker 2 — první ucelená verze (v1)

**Cíl v1:** kamarád v ní od prvního riffu po odeslaný mix udělá **celou písničku**. Do v1 patří jen to, bez čeho by se cesta 2.1 nebo 2.2 přerušila, a dále věci, které chrání nahrávky.

U hlavních funkcí jsou uvedeny tři věci: **Udělá** (co muzikant udělá), **Slyší / vidí** (co uslyší a uvidí) a **Ověří** (jak sám pozná, že vše funguje).

### 4.1 Zapojení a „Co slyším“

**Průvodce Zapojení** se spustí při prvním startu, později je dostupný z nabídky. **Rocker 2 nepředpokládá konkrétní vybavení.** Co jde, zjistí sám, na zbytek se zeptá jazykem muzikanta a nabízí jen volby, které s připojeným vybavením dávají smysl (kapitola 7). Má pět kroků a každý jde přeskočit:

1. **Zvuková karta.** Rocker 2 projde všechna zařízení a nabídne nejlepší ovladač: ASIO od výrobce, jinak exkluzivní režim Windows, jinak sdílený režim. Volbu vysvětlí lidsky: „Focusrite USB ASIO – nejmenší zpoždění“. Zároveň zjistí počet vstupů a výstupů a připojená zařízení MIDI.
2. **Sluchátka.** „Pustit zkušební tón“. Následuje otázka „Slyšíš ho v obou sluchátkách?“ s odpověďmi **[Ano] [Jen v jednom] [Ne]**. Ke každé odpovědi existuje konkrétní rada.
3. **Co máš zapojené?** „Zahraj na kytaru.“ Rozsvítí se vstup, na kterém je kytara, a Rocker 2 ho pojmenuje *Kytara*. Pak se jednou otázkou s obrázky zeptá na zdroj kytary (4.3). Následuje „Zahraj na bicí.“ Podle toho, kolik vstupů ožije a jestli přijdou údery přes MIDI, Rocker 2 sám nabídne odpovídající sestavu bicích (4.4).
4. **Zpoždění.** Tlačítko **Změřit zpoždění**. Uživatel propojí kabelem výstup zvukovky se vstupem a Rocker 2 zpoždění změří přesně (v dokumentu 03 zkouška M-01). Bez kabelu použije údaj ovladače a ukáže, že je jen „odhadnutý“.
5. **Přímý odposlech zvukovky.** „Má tvoje zvukovka knoflík MONITOR nebo DIRECT? Když hraješ přes aparát Rockeru, otoč ho na PLAYBACK/USB. Jinak uslyšíš kytaru dvakrát, čistou i se zkreslením.“

**Panel Co slyším / Co nahrávám** je trvale ve spodní desce režimu Hraj:

| Udělá | Slyší / vidí | Ověří |
|---|---|---|
| Zapne nahrávání stopy (●) a vybere jí vstup a odposlech: **Se zvukem / Čistě / Vypnuto**. | Panel vypisuje přesně, co jde do sluchátek (stopy, živé vstupy, klik) a co se nahraje (vstup → stopa → pokus č.). Dále ukazuje zpoždění v ms s hodnocením *dobré / použitelné / ruší* a zbývající místo na disku v hodinách nahrávání. | Ztlumí-li nějaký zdroj, zmizí ze sluchátek i ze seznamu. Hodnoty souhlasí s tím, co slyší. |

**Ochrany** [N]:

- Bluetooth sluchátka: „Bluetooth sluchátka mají obvykle zpoždění přes 100 ms. Na nahrávání použij sluchátka s kabelem.“
- Různé zařízení pro vstup a výstup: „Vstup a výstup jsou dvě různá zařízení. Nahrávky se mohou časem rozjíždět. Použij jednu zvukovku.“
- Zvukovka se odpojila: nahrávání se zastaví, vše nahrané do té chvíle zůstane a po připojení se zvukovka sama obnoví.

### 4.2 Rychlý záznam, Nápady, Pojistka a import

| Funkce | Udělá | Slyší / vidí | Ověří |
|---|---|---|---|
| **Nahrát nápad** | Stiskne R kdekoli na úvodní obrazovce nebo v knihovně Nápady. | Okamžitý záznam, bez odpočtu (pokud nemá zapnutý klik). Ve sluchátkách kytaru se zvukem posledně použité předvolby. | Po zastavení „Uloženo jako Nápad 18. 11. 21:40 (0:42)“. První úder není useknutý díky 2 s předstihu. |
| **Pojistka** (poslední 2 min) | Když chce uložit, co právě hrál, stiskne **U – Ulož, co jsem hrál**. | Ve stavovém pásu je drobná značka „Pojistka · 2:00“. Uloží se nápad dlouhý až 2 minuty. | Nápad obsahuje i to, co hrál před stiskem. Pojistku jde v nastavení vypnout. Zvuk z ní nikam neodchází a po zavření programu zmizí. |
| **Knihovna Nápady** | Filtruje ★, hledá v poznámkách, řadí podle data. | Řádek: náhled průběhu, délka, datum, tempo (je-li známé), ladění, poznámka. | Najde včerejší ★ nápad do 10 s. |
| **Udělat z toho skladbu** | Tlačítko u nápadu. | Nová skladba se stopou Kytara a s nabídkou Doťukat tempo. | Mřížka taktů sedí na první doby riffu. |
| **Import podkladu** | Přetáhne soubor do okna nebo zvolí **Přidat podklad**. Podporuje WAV, AIFF, FLAC, OGG, Opus, MP3; M4A/AAC jen přes kodeky Windows [Z]. | Vznikne nová stopa pojmenovaná podle souboru, umístěná na začátek nebo na ukazatel. Rocker 2 ji převede na vzorkovací frekvenci skladby. | Podklad hraje ve správné výšce i rychlosti. Šipkami jde posunout po 1 ms, s klávesou Shift po 10 ms, aby seděl na takt. |

> **Nahrávky ze starého Rockeru:** jednotlivé nahrávky stahoval jako `.webm` (Opus) ([recorder.ts ř. 17–22][rec-mime]). JUCE čte Opus jen v kontejneru Ogg, ne WebM. V1 je proto přímo neotevře. Mixy ze studia starého Rockeru (WAV) otevře. Import `.webm` je v rozšířeních. [Z]

### 4.3 Kytara

| Funkce | Udělá | Slyší / vidí | Ověří |
|---|---|---|---|
| **Ladička** (T) | Stiskne T, ladička se otevře přes okno. Zvolí ladění, výchozí je ladění skladby. | Velký ukazatel odchylky v centech, název tónu a struny. Řada strun, ve které se naladěné struny označí ✓. Potvrzení jen do sluchátek. | Všech 6 strun ✓. Na kontrolní tón z referenčního tónu ukáže ±0. |
| **Zvuk stopy** (aparát) | U stopy Kytara klikne na štítek zvuku (např. *Crunch*). Otevře se panel **Zvuk kytary** ve spodní desce. | Řetězec zleva doprava: **Brána → Zkreslení → Korekce → Reprobox → Ozvěna → Dozvuk → Hlasitost**. Jezdce mají srozumitelné názvy (*Zkreslení*, *Basy*, *Středy*, *Výšky*, *Jas*). | Posune Zkreslení a slyší změnu do 50 ms. Změna platí živě i pro to, co je už nahrané. |
| **Předvolby** | Vybere Čistý, Crunch, Lead, Metal nebo vlastní. **Uložit jako předvolbu**. | Předvolba se do stopy zkopíruje a štítek ukazuje její název. Po ruční úpravě má hvězdičku: *Crunch\**. | Skladba zní stejně i po smazání předvolby z knihovny. |
| **Hraní přes aparát** | Odposlech stopy: **Se zvukem**. | Kytara zní ve sluchátkách se zkreslením. Zpoždění je v panelu Co slyším. | [Z] Při bufferu 64–128 vzorků nesmí zpoždění při hře rušit (zkouška H-01). |
| **Brána** | **Změř ticho**: 2 s nehraje a Rocker 2 nastaví práh. | Mezi frázemi nebzučí. | Tlumené dohrávky se neusekávají. |
| **Jiný zvuk po nahrání** | Kdykoli změní předvolbu nebo jezdec. | Nahrávka hned zní jinak. | Přepnutí Crunch → Metal → Crunch vrátí přesně původní zvuk. |
| **Zdroj kytary** | V Zapojení nebo u stopy zvolí, odkud kytara přichází: <br>• **Přímo do zvukovky** (zvuk dělá aparát Rockeru 2) <br>• **Kombo s mikrofonem** <br>• **Linkový výstup komba nebo multiefektu** <br>• **Obojí zároveň**: čistá kytara do jednoho vstupu, kombo do druhého. Vzniknou dvě stopy ve skupině se společnými pokusy a střihy, stejně jako u Sady bicích. | U komba a multiefektu je zvuk stopy **žádný** a odposlech **Vypnuto**. Hráč slyší kombo z místnosti nebo přes přímý odposlech zvukovky. U volby *Obojí zároveň* slyší kombo a čistou stopu může později proměnit aparátem Rockeru 2. | Nahraje se to, co slyšel. U volby *Obojí zároveň* obě stopy sedí na sebe. |

### 4.4 Bicí

| Sestava | Jak to v Rockeru 2 funguje | Co slyší ve sluchátkách |
|---|---|---|
| **Jeden mikrofon** | Stopa *Bicí*, mono, jeden vstup. Rocker 2 doporučí umístění mikrofonu (asi 1–1,5 m před soupravou ve výšce hlavy) a test nejsilnějšího úderu („Udeř nejsilněji, jak budeš hrát“). Špička má zůstat pod −6 dBFS. | Akustické bicí slyší z místnosti. Odposlech stopy je ve výchozím stavu **Vypnuto**, takže ve sluchátkách jsou jen ostatní stopy a klik. |
| **Elektronické bicí stereo** | Výstup modulu L/R jde do dvou vstupů zvukovky. Stopa *Bicí* je **stereo** a je přiřazená ke vstupům 1+2. | Výchozí volba je odposlech přes Rocker 2 (vyžaduje nízké zpoždění), nebo přímý odposlech zvukovky. Sluchátka patří do zvukovky, ne do modulu, aby slyšel i ostatní stopy. |
| **Více mikrofonů** | **Sada bicích**: jedna skupina, v ní stopy *Kopák*, *Virbl*, *Tomy*, *Overhead L/P*… Každá stopa má vlastní vstup. Všechny se nahrávají současně a sdílejí pokusy i střihy, takže zůstávají přesně srovnané. Mix má každou zvlášť i skupinový jezdec *Bicí celkem*. Overheady mají předvolbu poloh vlevo a vpravo. | Jako u jednoho mikrofonu. |
| **Elektronické bicí přes MIDI** (modul posílá jen informace o úderech, nebo chce kamarád zvuk bicích měnit po nahrání) | **MIDI bicí – základ (součást v1).** Rocker 2 sám pozná modul připojený přes USB nebo kabel MIDI a nabídne stopu *Bicí (MIDI)*. Zaznamená údery (který pad, jak silně, kdy) a přehraje je **vestavěnou sadou bicích**. Sadu jde vyměnit i po nahrání, stejně jako zvuk kytary. Funkce **Naučit pady** („Udeř do kopáku… teď virbl…“) přiřadí pady i u modulu s neobvyklým rozložením. Oprava taktu, pokusy, díly i střih fungují stejně jako u zvukových stop. Úpravy jednotlivých úderů jsou v rozšířeních (kapitola 6); ve v1 se chybný takt opraví novým pokusem (**P**). | Údery zní ve sluchátkách přes vestavěnou sadu. Zpoždění tvoří jen výstup zvukovky, takže bývá malé. |
| **Elektronické bicí zvukem i MIDI** | Když modul posílá obojí, Rocker 2 nahraje **obojí zároveň**: zvuk modulu jako stereo stopu a údery jako MIDI. Kamarád si později vybere, co použije. | Podle volby zvuk modulu, nebo vestavěná sada. |

**Pohled zdálky** (F11) se při nahrávání bicích nabídne sám: „Zapnout Pohled zdálky při nahrávání bicích?“ (5.6).

**Ověří:** udeří do každého bubnu zvlášť a v Sadě bicích se rozsvítí jen správná stopa. Po nahrání a oříznutí taktu skupiny se ve všech stopách sady střihne totéž místo.

**Sestavu nikdo nemusí znát předem.** Rocker 2 nabídne jen sestavy, které dávají smysl s počtem vstupů a připojenými zařízeními MIDI. Se 2 vstupy například nenabídne Sadu bicích s osmi mikrofony a místo toho poradí: „Na víc mikrofonů potřebuješ zvukovku s víc vstupy. Zatím nahraj bicí jedním mikrofonem.“ [Z] Sestavu, kterou kamarád skutečně má, ověří zkouška H-07 (a u MIDI H-13).

### 4.5 Stopy a časová osa

| Funkce | Udělá | Slyší / vidí | Ověří |
|---|---|---|---|
| **Stopy** | **+ Přidat stopu** a zvolí druh *Kytara / Bicí / Sada bicích / Podklad / Jiné*. Dvojklikem na název ho přepíše („Rytmická“, „Sólo“). | Řádek stopy: barevný proužek podle druhu, název, vstup, ● nahrávat, Sólo, Ztlumit. Kytara navíc štítek zvuku. | — |
| **Nahrávání při poslechu stop** | Zapne ● u jedné nebo více stop a stiskne R. | Odpočet, pak hrají ostatní stopy a klik. Nahrávaná stopa kreslí průběh živě. | Nová stopa sedí na takty (kompenzace zpoždění, dokument 02, kap. 7 a 9). |
| **Díly** | Na pásu Díly: **Rozdělit díl zde**, přejmenovat (*Úvod, Sloka, Refrén, Most, Sólo, Závěr* nebo vlastní), barva. | Barevné „pásky“ nad stopami s názvem a délkou v taktech. | Kliknutím na díl se vybere celý (všechny stopy a takty). |
| **Vystřihnout, přesunout, zopakovat** | **X** rozstřihne klip na ukazateli. Tažením přesouvá (přichytává se na doby a takty, s klávesou Alt volně). **D** zopakuje výběr za sebou. **Delete** smaže. Ctrl+C a Ctrl+V. | Při tažení okraje se objeví **lupa** se 4× přiblížením okolí a mřížkou. | Klip začíná přesně na čáře taktu (lupa to ukáže). |
| **Zopakovat díl** | **D** na dílu nebo tažení s Ctrl. Vznikne spojený výskyt ⧉. | Viz 2.2, krok 4. | — |
| **Zpět a znovu** | **Ctrl+Z / Ctrl+Y**, neomezeně v rámci sezení. **Historie verzí** (Soubor → Vrátit skladbu do stavu z…) obsahuje uložené stavy z dřívějška, i z předchozích dní. | Krátká hláška „Zpět: Zkrácení Refrénu (2)“. | Po 10× Ctrl+Z a 10× Ctrl+Y je skladba stejná jako předtím. |
| **Smyčka** | **L** zapne přehrávání výběru dokola. | Na pravítku je pruh smyčky. | Přechod konec → začátek je plynulý, bez mezery. |

**Pravidla, která drží časovou osu jednoduchou** [N]:

- **Jedno tempo a jedno metrum na skladbu ve v1.** Tempo jde měnit volně, dokud je skladba prázdná. Po prvním nahrání Rocker 2 upozorní: „Nahrávky se změnou tempa nezrychlí. Mřížka by přestala sedět.“ Změny tempa uprostřed skladby jsou v rozšířeních.
- **Nic se nemaže z disku.** Střih, zkrácení i smazání klipu jen mění, kterou část nahrávky skladba přehrává. Nepoužité nahrávky se uklidí jen na výslovný pokyn **Uklidit skladbu**, a to do Koše Windows.

### 4.6 Oprava taktu a pokusy

Celý průběh je popsán ve 2.2, krok 3. Doplňující pravidla:

- **Výběr:** klik na číslo taktu vybere takt. Shift+klik rozšíří výběr na rozsah taktů. Výběr platí pro vybranou stopu, nebo pro celou Sadu bicích.
- **Předstih a dohra:** předstih 2 takty a dohra 1 takt, obojí jde změnit v nastavení. Odpočet podle nastavení skladby.
- **Nahrává se celý průchod**, ne jen opravovaný takt. Ve skladbě je slyšet jen opravovaný úsek, ale okraj pokusu jde roztáhnout. Když kamarád začal hrát o dobu dřív a povedlo se to, nic se neztratilo. [N]
- **Pokusy jsou ke stopě vázané místem.** Štítek **Pokusy 1 | 2 | 3** leží nad úsekem. **Tab** přepne na další pokus za běhu, **Enter** ponechá. Pravým tlačítkem na pokus: **Zahodit pokus** (jde do koše skladby a lze ho obnovit).
- **Ve spojeném dílu** se před opravou zeptá: *Opravit oba / Jen tento výskyt* (2.2, krok 4).

### 4.7 Mix a poslech

| Funkce | Udělá | Slyší / vidí | Ověří |
|---|---|---|---|
| **Hlasitost** | Táhne vodorovný jezdec v řádku stopy. Dvojklik vrátí 0 dB. | Hodnota v dB (−∞ … +6). | — |
| **Poloha ve stereu** | Jezdec L … střed … P. Dvojklik vrátí střed. | Mono stopa se posouvá mezi sluchátky. Stereo stopa mění vyvážení. | Úplně vlevo zní stopa jen v levém sluchátku. |
| **Sólo / Ztlumit** | **S / M** na vybrané stopě, nebo tlačítka. | Když je zapnuté některé sólo, stavový pás trvale ukazuje „SÓLO: Kytara“, aby nezapomněl. | — |
| **Přebuzení** | — | Dva druhy hlášení: **PŘEBUZENÝ VSTUP** (nastal při nahrávání, nedá se opravit, je potřeba ubrat gain a nahrát znovu) a **PŘEBUZENÝ MIX** (ubrat jezdce, opravitelné). U měřiče je značka, kde k přebuzení došlo: „takt 37“. | Klik na značku skočí na místo přebuzení. |
| **Celkem (master)** | Jezdec celkové hlasitosti a velký měřič. | Špička v dBFS a místo nejhlasitějšího momentu. | Export neukáže přebuzení, pokud ho neukázal měřič při přehrání. |
| **Export** | **Ctrl+E – Poslat mix**. Celá skladba nebo výběr. WAV 24 bit / 48 kHz nebo MP3 320 kb/s. | Soubor se vytvoří rychleji než v reálném čase a otevře se jeho složka. | **Export = co slyšel:** stejný zvuk stop, aparát, hlasitosti a polohy. **Bez kliku**, ztlumené stopy tiché a při sólu upozornění. Tlačítko **Přehrát výsledný soubor**. |

### 4.8 Uložení, přenos a záchrana

| Situace | Co Rocker 2 dělá | Co muzikant vidí |
|---|---|---|
| **Běžná práce** | Ukládá po každé dokončené úpravě (se zpožděním 1–2 s) bezpečným způsobem: nejdřív nový soubor, pak záměna, takže rozepsaný soubor nikdy nepřepíše dobrý. Každých 5 minut práce a při zavření uloží verzi do historie. | „Uloženo ✓“ ve stavovém pásu. |
| **Zítra** | Pamatuje si i pohled: režim, přiblížení, ukazatel, vybranou stopu a smyčku. | „Pokračovat: Noční riff“ na úvodní obrazovce. |
| **Jiný počítač** | **Soubor → Zabalit skladbu** vytvoří jeden soubor `.rocker2` se vším zvukem. Na druhém počítači **Otevřít** soubor rozbalí do složky Skladby. Kopírování celé složky skladby (USB, cloud) funguje také. | Na novém počítači se Rocker 2 zeptá na přiřazení vstupů podle názvů stop („Kytara byla na *Vstupu 1* zvukovky Focusrite. Kde je teď?“). |
| **Poslech jinde** | Export WAV nebo MP3 (4.7). | — |
| **Dochází místo** | Před nahráváním ukazuje zbývající čas („Místo na disku: 41 h nahrávání“). Pod 30 minut varuje. Pod bezpečnou mez (asi 500 MB) nahrávání **řízeně ukončí** a vše nahrané do té chvíle zůstane. | „Nahrávání zastaveno: dochází místo na disku. Vše do 3:12 je uloženo.“ |
| **Program spadne uprostřed nahrávky** | Nahrávka se zapisuje na disk průběžně a nejméně jednou za sekundu se zkontroluje, aby byl soubor čitelný. Při dalším startu Rocker 2 najde nedokončený záznam. | „Rocker 2 se minule nečekaně ukončil během nahrávání. Zachránil jsem 3 min 12 s stopy Kytara (pokus 4). **[Vložit zpět do skladby]** **[Uložit jako nápad]**“. Ztratí se nejvýš poslední 1–2 sekundy. [Z] |
| **Výpadek proudu** | Jako pád programu. Windows ale může mít v mezipaměti poslední sekundy, které se na disk nedostanou. | [Z] Ověřit zkouškou M-07. |
| **Spánek notebooku** | Během nahrávání a přehrávání Rocker 2 nedovolí Windows usnout. | — |
| **OneDrive** | Pokud má složka Skladby v cestě OneDrive, Rocker 2 doporučí jiné umístění. Synchronizace velkých zvukových souborů během nahrávání může zápis zdržovat a OneDrive často přesměrovává Dokumenty a Plochu. | „Složka je synchronizovaná přes OneDrive. Pro nahrávání doporučuji `C:\Users\<jméno>\Rocker 2`.“ |

---

## 5. Vzhled a rozložení Rockeru 2 (nový návrh)

### 5.1 Zásady odvozené z práce muzikanta

1. **Stav nahrávání je vidět periferním viděním.** Bubeník se dívá na činely, ne na notebook. Proto se při záznamu barví *celé okno*: červený stavový pás a 6px červený rám kolem okna. V Pohledu zdálky je červená celá obrazovka. Barevná plocha se čte z 3 m lépe než text.
2. **Stopy se nikdy nehýbou.** Ve všech režimech (Hraj, Stříhej, Míchej) je stopa na stejném řádku a ve stejné barvě. Mění se jen to, co je v řádku vidět. Muzikant si pamatuje „kytara je druhá shora“.
3. **Mix v řádcích, ne ve sloupcích.** Notebook má široký, ale nízký displej (1280×720 logických bodů při 150% zvětšení u 15" Full HD). Svislé jezdce by se nevešly a nesouhlasily by s řádky stop. Proto má každá stopa vodorovný jezdec přímo ve svém řádku. [N]
4. **Přesnost tam, kde se stříhá.** Čísla taktů jsou velká klikací pole (nejméně 32 px široká). Okraje se přichytávají na mřížku a při tažení se ukáže lupa.
5. **Jedna velká klávesa na jednu věc.** Ovládání s nástrojem v ruce používá jen klávesy bez modifikátorů, které jsou velké nebo na okraji klávesnice (5.10).
6. **Jedno okno, žádné překryvné dialogy při hraní.** Hlášky se objevují ve stavovém pásu nebo ve spodní desce a nezastaví přehrávání. Potvrzovací okno je jen u nevratných kroků (a těch je málo, protože skoro vše jde vrátit).
7. **Klid, který nekřičí.** Tmavá grafitová plocha, která večer neoslňuje. Barva je vyhrazená pro význam: druh stopy, díl, stav nahrávání a varování.

### 5.2 Vizuální jazyk

**Myšlenka: „Páska a křída“.** Muzikanti si ve zkušebně popisují kabely, kanály na pultu a setlisty proužky lepicí pásky. Rocker 2 z toho bere jeden výrazný prvek: **díly skladby a štítky stop vypadají jako proužky pásky** (obdélník se zkoseným rohem 45°). Vše ostatní je ploché, technické a čitelné, s textem v teplé „křídově“ bílé na grafitu. [N]

**Barvy** (pro implementaci jako pojmenované tokeny, jen tmavý režim ve v1):

| Token | Hodnota | Použití |
|---|---|---|
| `podklad` | `#121417` | pozadí okna |
| `plocha` | `#1B1E23` | panely, hlavy stop |
| `plocha-vyssi` | `#242830` | tlačítka, aktivní řádek |
| `linka` | `#333843` | oddělovače |
| `mrizka-takt` / `mrizka-doba` | `#3A404C` / `#262A31` | čáry taktů a dob |
| `text` | `#EDE9E3` | hlavní text („křída“) |
| `text-tlumeny` | `#A9ADB5` | popisky |
| `text-slaby` | `#757B86` | nápovědy, neaktivní |
| `zaznam` | `#FF3B3B` | ● Nahrát, rám okna při záznamu |
| `zaznam-plocha` | `#B3141B` | stavový pás a Pohled zdálky při záznamu |
| `ukazatel` | `#F7F4EE` | svislá čára přehrávání |
| `vyber` | `#4C8DFF` (výplň 22 %) | vybrané takty |
| `pozor` | `#FFC233` | blízko přebuzení, upozornění |
| `prebuzeni` | `#FF6A1A` | přebuzeno (vždy i s nápisem PŘEBUZENO) |
| `v-poradku` | `#3CCB7F` | naladěno, uloženo, připraveno |

**Barvy druhů stop:** Bicí `#2EC4B6`, Kytara `#F2A541`, Kytara 2 `#E86A92`, Basa `#8F7CF7`, Hlas `#5BB8F5`, Podklad `#9AA3AF`, Jiné `#B7C46A`.
**Barvy pásek dílů:** Úvod `#7C86FF`, Sloka `#3FB6A8`, Refrén `#F0B429`, Most `#E5737A`, Sólo `#B884F5`, Závěr `#8D99A6`.

Pravidlo: **žádný stav se nesděluje jen barvou.** Nahrávání má kruh ●, slovo NAHRÁVÁM a rám. Přebuzení má nápis. Naladěno má ✓.

**Typografie:**

- **Inter** (licence SIL OFL 1.1) pro rozhraní: 14 px základ, 12 px popisky, 11 px VERZÁLKY s prostrkáním +6 % pro nadpisy sekcí.
- **JetBrains Mono** (SIL OFL 1.1) pro **všechna čísla**: takty, čas, dB, BPM. Číslice mají stejnou šířku, takže počítadlo „neposkakuje“.
- Velikosti: počítadlo ve stavovém pásu 28 px, číslo taktu v Pohledu zdálky 180 px, nápis NAHRÁVÁM 96 px.
- Fonty se vloží do aplikace, aby vypadala stejně na každém počítači. [Z] Ověřit vykreslení ř, ů, ě, ť ve všech velikostech.

**Tvary a rozměry:** mřížka 4 px. Tlačítka mají výšku 32 px, hlavní transport 44 px a ● Nahrát 52 px. Rohy mají poloměr 4 px; páskové štítky místo zaoblení zkosený roh 45° (8 px). Minimální klikací plocha je 32×32 px.

**Ikony:** vlastní sada, čárové, tah 2 px na mřížce 20×20, rovná zakončení, bez výplní (výjimka: plný kruh ● Nahrát). Seznam pro v1: Nahrát (plný kruh), Přehrát (trojúhelník), Zastavit (čtverec), Smyčka (dvě protiběžné šipky v obdélníku), Klik (svislá čárka nad vodorovnou s tečkou: „úder na dobu“), Odpočet (číslice 4 v kroužku), Ladička (ladicí vidlička), Zvuk kytary (kroutítko se zářezem), Nůžky, Zopakovat (dva překrývající se obdélníky), Spojený výskyt ⧉, Pojistka (kruhová šipka se značkou 2:00), Zdálky (rozbalovací rohy), Uloženo (✓ v kosočtverci), Varování (trojúhelník s !), Vstup (zástrčka jack).

### 5.3 Hlavní okno — režim **Hraj** (nahrávání)

Náčrt pro běžný notebook (1280×720 logických bodů). Kamarád právě nahrává kytaru přes bicí.

```
┌────────────────────────────────────┬──────────────────────────────┬────────────────────────┐
│ ROCKER 2   Noční riff ▾   Uloženo ✓│ ● NAHRÁVÁM  Kytara · pokus 3 │  TAKT  6.2    0:11,8   │
│ [1 Hraj] [2 Stříhej] [3 Míchej]    │  ■ Zastavit   ▶   ⟲ Smyčka   │  112 BPM · 4/4 · Klik ◉│
├───────────────────────┬────────────┴────────────────────┬─────────┴────────────────────────┤
│ DÍLY                  │ ◤ Sloka ─────────────────────── │ ◤ Refrén ─────────────────────── │
│ TAKTY                 │ 1      2      3      4      5   │  6▼    7      8      9     10    │
├───────────────────────┼─────────────────────────────────┼──────────────────────────────────┤
│▌BICÍ          ○ S M   │▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆│▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇│
│  Vstupy 3–4 · stereo  │                                 │                                  │
├───────────────────────┼─────────────────────────────────┼──────────────────────────────────┤
│▌KYTARA        ● S M   │▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃│▃▅▆▅▃▅▆▅▒▒▒▒░░░░                  │
│  Vstup 1 · ◤Crunch    │                                 │  (kreslí se živě)                │
├───────────────────────┴─────────────────────────────────┴──────────────────────────────────┤
│ + Přidat stopu                                                                             │
├──────────────────────────────────┬────────────────────────────────┬────────────────────────┤
│ CO SLYŠÍM                        │ CO NAHRÁVÁM                    │ VSTUPY                 │
│ Bicí – stopa          ▕█████░▏   │ Vstup 1 → Kytara · pokus 3     │ 1 Kytara ▕██████░░▏ −9 │
│ Kytara – živě, Crunch ▕████░░▏   │ Zpoždění: 7 ms – dobré ✓       │ 2 —      ▕░░░░░░░░▏    │
│ Klik                  ▕██░░░░▏   │ Místo na disku: 41 h           │ 3–4 Bicí ▕░░░░░░░░▏    │
│ [T Ladička]  [Zvuk kytary]       │ Pojistka · 2:00                │ [Změřit zpoždění]      │
└──────────────────────────────────┴────────────────────────────────┴────────────────────────┘
  ▲ při nahrávání je celý stavový pás červený a okno má 6px červený rám
```

**Proč jsou prvky právě tady:**

- **Stavový pás nahoře (64 px)** je místo, kam oko jde jako první. Uprostřed je stav a transport, vpravo velké počítadlo taktu, protože muzikant potřebuje vědět „kde jsem“ víc než „kolik je hodin“. Při záznamu zčervená celý pás.
- **Přepínač režimů vlevo nahoře** s čísly 1/2/3 stejnými jako klávesy, takže zkratku se naučí z pohledu.
- **Hlavy stop vlevo (200 px)**: ● nahrávat je nejblíž k názvu, protože je to nejčastější volba v režimu Hraj. Vstup a zvuk kytary jsou pod názvem, aby šlo jedním pohledem zkontrolovat „co se kam nahraje“.
- **Díly a takty nad stopami**: struktura skladby je nad obsahem, stejně jako nadpis nad textem.
- **Spodní deska (asi 150 px)** obsahuje „Co slyším / Co nahrávám / Vstupy“. Jsou to odpovědi na otázky, které si muzikant klade *před* stiskem R, proto jsou v režimu Hraj trvale vidět.

### 5.4 Režim **Stříhej** — oprava taktu uprostřed skladby

```
┌────────────────────────────────────┬────────────────────────┬──────────────────────────────┐
│ ROCKER 2   Noční riff ▾   Uloženo ✓│  ▶ Přehrát  ⟲ Smyčka   │  TAKT 22.1   0:45,0          │
│ [1 Hraj] [2 Stříhej] [3 Míchej]    │  P Oprav takt          │  112 BPM · 4/4 · Klik ◉      │
├───────────────────────┬────────────┴─────────────────┬──────┴──────────────────────────────┤
│ DÍLY                  │ ◤ Refrén ⧉ ───────────────── │ ◤ Sloka 2 ───────────────────────── │
│ TAKTY                 │  19    20    21  ┃ 22 ┃  23  │  24    25    26    27    28    29   │
├───────────────────────┼──────────────────────────────┼─────────────────────────────────────┤
│▌BICÍ                  │▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇┃▅▆▇▆┃▆▇▆▅▆▇│▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆│
├───────────────────────┼──────────────────────────────┼─────────────────────────────────────┤
│▌KYTARA  ◤Crunch       │▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅┃▅▃▅▆┃▃▅▆▅▃▅│▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃│
│                       │            Pokusy [1][2●][3] │  ← štítek nad úsekem                │
├───────────────────────┴──────────────────┬───────────┴─────────────────────────────────────┤
│ POKUSY · Kytara · takt 22                │ VÝBĚR                                           │
│  1  ▃▅▆▅▃▅▆   včera 23:02                │ Takt 22 · Kytara · 2,14 s                       │
│  2● ▃▅▆▅▃▅▆   dnes 10:15   ← zní         │ Předstih 2 takty · Dohra 1 takt · Odpočet ◉     │
│  3  ▃▅▆▅▃▅▆   dnes 10:16                 │ Refrén se hraje 2×: oprava platí pro oba        │
│ Tab další · Enter ponechat · P nový      │ [Opravit oba]  [Jen tento výskyt]               │
└──────────────────────────────────────────┴─────────────────────────────────────────────────┘
```

**Proč takhle:** čísla taktů jsou nejvýraznější prvek pravítka a klik na ně vybírá. Vybraný takt je zvýrazněný přes všechny stopy, ale opravuje se jen vybraná stopa (zvýrazněná hlava). Seznam pokusů ve spodní desce ukazuje i *kdy* který pokus vznikl, protože muzikant si pamatuje „ten ráno byl lepší“. Upozornění na spojený refrén je u tlačítek, kterých se týká, a ne v okně, které by zastavilo práci.

**Lupa při tažení okraje** (objeví se nad kurzorem, 4× přiblížení):

```
          ┌──────────────── LUPA ×4 ────────────────┐
          │ doba 4 (takt 21)  ┃ takt 22 ┊ doba 2     │
          │  ▁▂▃▅▇▆▅▃▂▁▁▂▃▅▇▆ ┃ ▅▃▂▁▂▃▅▇ ┊ ▆▅▃▂▁      │
          │                   ┃◀ okraj se přichytí  │
          └─────────────────────────────────────────┘
```

### 5.5 Režim **Míchej**

```
┌────────────────────────────────────┬────────────────────────┬──────────────────────────────┐
│ ROCKER 2   Noční riff ▾   Uloženo ✓│  ▶ Přehrát  ⟲ Smyčka   │  TAKT 9.1   0:17,1           │
│ [1 Hraj] [2 Stříhej] [3 Míchej]    │  SÓLO: nic             │  Ctrl+E Poslat mix           │
├────────────────────────────────────┴─────────────────┬──────┴──────────────────────────────┤
│ STOPA         HLASITOST             POLOHA      S  M │ DÍLY  ◤Sloka   ◤Refrén  ◤Sloka 2    │
├──────────────────────────────────────────────────────┼─────────────────────────────────────┤
│▌BICÍ   −2,0 ━━━━━━━━━━━━●━━━━  L ━━━●━━━ P   ○  ○    │▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆▅▆▇▆  │
│        ▕████████████▓▓░░░░▏ −8 dB                    │                                     │
├──────────────────────────────────────────────────────┼─────────────────────────────────────┤
│▌KYTARA −4,5 ━━━━━━━━━━●━━━━━━  L ━━●━━━━ P   ○  ○    │▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆▅▃▅▆  │
│  ◤Crunch ▕██████████▓░░░░░░░▏ −11 dB                 │                                     │
├──────────────────────────────────────────────────────┴─────────────────────────────────────┤
│ CELKEM  0,0 ━━━━━━━━━━━━━━●━━  ▕L ██████████████▓▓░░▏  špička −3,1 dBFS (takt 14)          │
│                                ▕P █████████████▓▓▓░░▏  Přebuzení: žádné ✓                  │
│ [Poslat mix…]   Formát: MP3 320 / WAV 24 bit    Rozsah: celá skladba / výběr               │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

**Proč takhle:** hlavy stop se rozšíří doprava o jezdce, takže stopa zůstává na svém řádku (zásada 2) a časová osa je dál vidět, i když užší. Je tak vidět, *kde* ve skladbě se poslouchá. Celkový měřič je dole přes celou šířku, protože je to poslední kontrola před exportem. Hned vedle je tlačítko Poslat mix.

### 5.6 **Pohled zdálky** (F11)

Celá obrazovka, čitelná ze 2–3 m. Barva plochy nese hlavní informaci a text je druhý v pořadí.

```
 PŘIPRAVENO (tmavá)       ODPOČET (jantarová)       NAHRÁVÁM (celá plocha červená)
┌────────────────────┐    ┌────────────────────┐    ┌────────────────────────────────┐
│ ● připraveno: Bicí │    │                    │    │ ● NAHRÁVÁM         Bicí · p.1  │
│                    │    │        ███         │    │                                │
│   TAKT 1           │    │         ██   3     │    │          TAKT  5 / 8           │
│                    │    │        ███         │    │                                │
│ ▢ ▢ ▢ ▢   112 BPM  │    │  ■ ■ ▢ ▢           │    │   ■ ■ ▢ ▢     0:09   112 BPM   │
│ R = nahrát         │    │                    │    │   R / Enter = zastavit         │
└────────────────────┘    └────────────────────┘    └────────────────────────────────┘
                                                    po zastavení: zelená „ULOŽENO ✓ pokus 1“
```

- Čtverce ▢ jsou doby taktu; právě znějící doba se vyplní. Bubeník tak vidí puls i bez sluchátek.
- Číslo taktu „5 / 8“ se zobrazí, pokud je nastavená smyčka nebo cílová délka (8 taktů). Jinak se ukáže jen číslo taktu.
- Esc nebo F11 pohled zavře. Nahrávání tím nezastaví.

### 5.7 Úvodní obrazovka

```
┌────────────────────────────────────────────────────────────────────────────────────────────┐
│ ROCKER 2                                                      Zvukovka: Focusrite ASIO ✓   │
│                                                                                            │
│   ┌──────────────────────────────┐   ┌──────────────────────────────┐                      │
│   │  ●  NAHRÁT NÁPAD        (R)  │   │  +  NOVÁ SKLADBA        (N)  │                      │
│   └──────────────────────────────┘   └──────────────────────────────┘                      │
│                                                                                            │
│   POKRAČOVAT                                                                               │
│   ▸ Noční riff         včera 23:12 · takt 22 · Drop D · 112 BPM     [Enter]                │
│     Pomalá ve Standardu   před 3 dny                                                       │
│                                                                                            │
│   NÁPADY  (★ 4 · celkem 17)                              [Otevřít knihovnu]                │
│   ★ Nápad 18. 11. 21:40   0:42  ▃▅▆▅▃▅▆▅▃   „Drop D, svižně, sloka?“                       │
│     Nápad 17. 11. 19:02   1:55  ▂▃▅▃▂▃▅▃▂                                                  │
│                                                                                            │
│   [Zapojení a zvukovka]   [Ladička T]    Pojistka · 2:00 · U = ulož, co jsem hrál          │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 5.8 Cesta od otevření programu k první nahrávce

1. **První spuštění:** otevře se **Zapojení** (4.1). Trvá asi 2 minuty a každý krok jde přeskočit.
2. **Úvodní obrazovka** (5.7). Kurzor ani myš nejsou potřeba.
3. **R** spustí **Nahrát nápad**. Okno se přepne do zjednodušeného režimu Hraj se stopou *Kytara*. Stavový pás zčervená, nahrávání běží a obsahuje i 2 s před stiskem.
4. Hraje. Uprostřed okna se kreslí průběh a vpravo nahoře běží čas.
5. **R** (nebo Mezerník) zastaví. Následuje hláška „Uloženo jako Nápad 18. 11. 21:40 (0:42)“ a nabídka **[▶ Přehrát] [★] [Udělat z toho skladbu]**.

Pro skladbu od bicích: **N** (Nová skladba) → název a tempo, případně jen Enter → dvě stopy připravené podle zapojení (Bicí, Kytara) → ● u Bicí → **F11** → **R**.

### 5.9 Cesta k opravě jednoho taktu uprostřed skladby

1. **2** (Stříhej). Klik na hlavu stopy *Kytara*.
2. Klik na číslo **22** na pravítku (Shift+klik na 23 pro dva takty).
3. **P**: předstih od taktu 20, odpočet, nahrávání taktu 22, dohra taktu 23, stop.
4. Automatický poslech taktů 21–23 s novým pokusem. **Tab** přepíná pokusy na stejném místě.
5. **Enter** ponechá. Hotovo. Celou dobu zůstávají ruce u nástroje kromě dvou kliknutí na začátku.

### 5.10 Názvy tlačítek a klávesové zkratky

**Pro chvíli, kdy jsou ruce na nástroji** (jen klávesy bez modifikátorů):

| Klávesa | Tlačítko | Co udělá |
|---|---|---|
| **Mezerník** | ▶ Přehrát / ■ Zastavit | Spustí nebo zastaví přehrávání; při nahrávání zastaví a pokus uloží. |
| **R** | ● Nahrát | Spustí nahrávání s odpočtem. Druhé stisknutí zastaví a uloží. |
| **P** | Oprav takt | Nahraje znovu jen vybrané takty, s předstihem a dohrou. |
| **Tab** | Další pokus | Přepne na další pokus, přehrávání běží dál. |
| **Enter** | Ponechat tenhle | Ponechá právě znějící pokus. |
| **U** | Ulož, co jsem hrál | Uloží poslední 2 minuty z Pojistky jako nápad. |
| **K** | Klik | Zapne nebo vypne klik ve sluchátkách. |
| **L** | Smyčka | Přehrává výběr dokola. |
| **T** | Ladička | Otevře nebo zavře ladičku. |
| **F11** | Zdálky | Pohled zdálky. |

**Numerická klávesnice u bicích** [N]: levná USB numerická klávesnice položená u soupravy se dá ovládat špičkou paličky.

| Klávesa | Funkce |
|---|---|
| **Num 0** | Přehrát / Zastavit |
| **Num Enter** | Nahrát / Zastavit |
| **Num +** | Další pokus |
| **Num −** | Zpět o takt |

**Nožní spínač:** ve v1 funguje každý USB nožní spínač, který se v počítači hlásí jako klávesnice. V nastavení se mu přiřadí klávesa R. Spínač s MIDI a pad elektronických bicích jako tlačítko Nahrát jsou ve v1.1.

**U klávesnice:**

| Klávesa | Funkce |
|---|---|
| **1 / 2 / 3** | Režim Hraj / Stříhej / Míchej |
| **← / →** | O takt zpět / vpřed |
| **Home / End** | Začátek / konec |
| **X** | Rozstřihnout na ukazateli |
| **D** | Zopakovat výběr nebo díl |
| **Delete** | Smazat výběr (jde vrátit) |
| **S / M** | Sólo / Ztlumit vybranou stopu |
| **+ / −** | Přiblížit / oddálit |
| **Alt + tažení** | Bez přichytávání |
| **Ctrl+Z / Ctrl+Y** | Zpět / Znovu |
| **Ctrl+S** | Uložit (pojistka, ukládá se samo) |
| **Ctrl+E** | Poslat mix |
| **N** | Nová skladba (na úvodní obrazovce) |

> **Esc nikdy nic nezahazuje.** Zastaví a pokus uloží. Pokus se zahazuje jen výslovně v panelu Pokusy a i pak jde z koše skladby obnovit.

### 5.11 Ukázky hlášek (tón a styl)

Hlášky jsou krátké, v druhé osobě a vždy říkají, co dělat dál.

- „Kytara je přebuzená už na vstupu. Ubrat GAIN na zvukovce. Tohle se v mixu opravit nedá.“
- „Zpoždění ve sluchátkách: 23 ms – při hraní může rušit. Zkus menší buffer: [128] [64].“
- „Sólo je zapnuté na stopě Kytara. Mix by obsahoval jen ji.“
- „Dochází místo: zbývá asi 25 minut nahrávání.“
- „Tato skladba je v Drop D. Ladička je nastavená.“
- „Zvukovka se odpojila. Nahrávání zastaveno, vše do 2:41 je uloženo.“
- „Refrén se hraje 2×. Oprava platí pro oba.“

---

## 6. Rozšíření po první verzi

Řazeno podle užitku pro tvorbu písně:

| # | Rozšíření | Co přinese | Proč až teď |
|---|---|---|---|
| 1 | **Nahrávání dokola** (smyčka → pokus nebo vrstva při každém průchodu) | Hledání riffu jako se smyčkovým pedálem, ale rovnou ve skladbě a s oddělenými vrstvami. „Udělat ze smyčky díl.“ | Staví na pokusech a smyčce z v1. |
| 2 | **Vodicí rytmus** místo kliku | Kytarista nahraje kytaru dřív než bicí a necítí se jako s budíkem. | Nápad z drum mode starého Rockeru. Pro v1 stačí klik. |
| 3 | **Nechat doznít přes hranu dílu** | Činely a dozvuky na konci výskytu dílu se neutnou. | Vyžaduje přesah klipů přes díl. |
| 4 | **Export stop zvlášť** | Poslat bicí a kytaru odděleně kamarádovi s jiným studiem. | Jednoduché, ale není nutné pro první písničku. |
| 5 | **Nožní spínač přes MIDI, pad bicích jako tlačítko Nahrát** | Bubeník spustí záznam úderem do padu. | Ve v1 funguje spínač, který se hlásí jako klávesnice. |
| 6 | **Spustit hraním** | Nahrávání začne samo, když kamarád začne hrát (díky Pojistce bez useknutí). | Potřebuje doladit práh podle skutečných zkoušek. |
| 7 | **Zmrazit zvuk kytary** | Úspora výkonu u slabého notebooku a možnost zapéct zvuk. | Teprve když bude výkon chybět. |
| 8 | **Vlastní reprobox (IR soubory)** a **profily aparátů NAM** (Neural Amp Modeler, open source, licence MIT) | Věrný zvuk konkrétních aparátů. | Vyšší výkonová náročnost a výběr profilů. Nejdřív ověřit vestavěný aparát. |
| 9 | **Úpravy jednotlivých úderů MIDI** (posun, síla, srovnání do rytmu) a další sady bicích | Doladit bicí bez nového nahrávání. | Ve v1 se chyba opraví novým pokusem (P), stejně jako u kytary. |
| 10 | **Pluginy VST3** | Vlastní efekty a nástroje. | Riziko pádů cizích pluginů a složitější testy. |
| 11 | **Změny tempa a metra**, swing kliku | Skladby s ritardandem nebo změnou metra. | Zesložití časovou osu. |
| 12 | **Hlasitost v čase** (automatizace), **hlasitější export** (omezovač), měření hlasitosti LUFS | Hotovější mix. | Po první písničce. |
| 13 | **Import `.webm` ze starého Rockeru**, nastavitelné komorní A | Pohodlí. | Málo časté. |
| 14 | **Zpomalené přehrávání vlastní nahrávky** pro nácvik sóla | Nácvik. | Mimo tvorbu. |

---

## 7. Neznámé vybavení: Rocker 2 počítá se všemi běžnými možnostmi

Kamarádovo vybavení zatím neznáme a vědět ho nemusíme. **Rocker 2 podporuje všechny běžné sestavy a při Zapojení (4.1) sám pozná, kterou kamarád má.** Dopředu se na nic neptá a nabízí jen volby, které s připojeným vybavením dávají smysl. [N]

| Co neznáme | Běžné možnosti, se kterými Rocker 2 počítá | Jak to Rocker 2 pozná | Co udělá |
|---|---|---|---|
| **Zvuková karta** | S ovladačem ASIO od výrobce · bez ASIO (jen ovladač Windows) · 1, 2, 4, 8 i víc vstupů · 44,1 / 48 / 96 kHz · **žádná zvukovka** (jen mikrofon a sluchátkový výstup notebooku) | Projde zařízení a ovladače a přečte počet vstupů a podporované frekvence. | Vybere nejlepší ovladač. Bez ASIO zkusí exkluzivní a nízkolatenční režim Windows a změří zpoždění. Kdyby zpoždění při hře přes aparát rušilo, nabídne **Přímý odposlech zvukovky**: kytara se nahraje čistá, hráč ji slyší bez zpoždění přímo ze zvukovky a zvuk aparátu nastaví až při přehrávání. Bez zvukovky funguje jako náhradní režim pro zachycení nápadu, s varováním, že na nahrávání přes aparát to nestačí. |
| **Bicí** | Akustické s 1 mikrofonem · akustické s více mikrofony · elektronické se zvukovým výstupem · elektronické přes MIDI · elektronické se zvukem i MIDI | Výzva „Zahraj na bicí“: kolik vstupů ožije a jestli přijdou údery MIDI. | Nabídne odpovídající sestavu (4.4). Když má kamarád víc mikrofonů než vstupů, poradí, co dál. |
| **Kytara** | Přímo do zvukovky · kombo s mikrofonem · linkový výstup komba nebo multiefektu · obojí zároveň | Jedna otázka s obrázky. | Nastaví zvuk stopy a odposlech (4.3). |
| **Notebook** | Windows 10 (verze 1607 a novější) nebo 11 · procesor Intel/AMD (x64) · ARM (Snapdragon) | Instalátor pozná procesor sám. | Nainstaluje odpovídající verzi; obě se sestavují pokaždé. U ARM upozorní, že ovladače ASIO pro ARM jsou vzácné, a použije režim Windows. |
| **Sluchátka** | Drátová do zvukovky · drátová do notebooku · Bluetooth | Podle zvoleného výstupu. | U Bluetooth varuje před zpožděním přes 100 ms. Když vstup jde ze zvukovky a výstup z notebooku, varuje před rozjížděním (4.1). |
| **Ovládání s nástrojem v ruce** | Klávesnice · USB nožní spínač, který se hlásí jako klávesnice · numerická klávesnice (v1.1 navíc pad bicích a spínač MIDI) | Výzva „Stiskni tlačítko, které chceš používat pro Nahrát“. | Přiřadí ho. |

**Co to stojí navíc.** Většinu z toho návrh už obsahoval. Nově přibývají tři věci:

- **MIDI bicí – základ**: jedna samostatná etapa (dokument 04, E10),
- volba zdroje kytary,
- sestavení pro ARM (drobnost).

**Co se tím nevyřeší.** Zařízení, která nikdo z nás nemá, můžeme ověřit jen simulací (dokument 03, A-24). Proto má Rocker 2 v Diagnostice **Kontrolu sestavy**: kamarád ji jednou spustí a dostane srozumitelný přehled, co na jeho počítači funguje. Výsledek pošle Jendovi (03, M-15).

**Odpovědi kamaráda už nejsou potřeba.** Pokud je někdy zjistíš, pomohou jen rozhodnout, kterou zkoušku udělat jako první.

## 8. Minimální vybavení

Rocker 2 musí fungovat s tímto minimem; vše navíc je výhoda:

- Notebook s Windows 10 (verze 1607 a novější) nebo 11, procesor x64 nebo ARM64, nejméně 8 GB paměti. SSD disk je doporučený.
- Jakákoli USB zvuková karta s aspoň jedním vstupem (pro kytaru ideálně nástrojovým, označeným INST nebo Hi-Z) a výstupem na sluchátka. Bez zvukovky funguje jen náhradní režim pro zachycení nápadu.
- Drátová sluchátka.
- Skladby do 10 minut a do 16 stop (zvukových i MIDI), 44,1–96 kHz (výchozí 48 kHz).
- Žádný internet ani účet. Rocker 2 nic neodesílá.

---

<!-- Odkazy na původní repozitář (commit 77df8e3, 3. 6. 2026) -->
[readme]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/README.md
[st-naslepo]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/PROJECT_STATUS.md#L44-L50
[st-mono]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/PROJECT_STATUS.md#L519-L529
[rm-naslepo]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/ROADMAP.md#L217-L234
[tuner-hook]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/tuner/useTuner.ts#L11-L16
[guided]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/tuner/GuidedTuner.tsx#L16-L20
[tunings]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/tunings.ts#L20-L129
[custom-tunings]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/customTunings.ts
[reftone]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/referenceTone.ts#L1-L20
[gate]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/pitchDetector.ts#L34-L51
[a440]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/pitch.ts#L3
[metro]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/metronome.ts#L1-L9
[metro-state]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/metronome.ts#L74-L80
[tempo]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/tempo.ts#L3-L4
[tap]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/tempo.ts#L13-L28
[signatures]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/metronomeConfig.ts#L100-L113
[swing]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/metronomeConfig.ts#L120-L123
[countin-max]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/metronomeConfig.ts#L164
[clicks]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/metronomeConfig.ts#L173
[trainers]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/metronomeConfig.ts#L250-L306
[metro-presets]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/metronomePresets.ts#L73
[countin]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/countIn.ts
[amp-chain]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/ampChain.ts#L1-L13
[amp-curves]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/ampChain.ts#L38-L84
[amp-reverb]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/ampChain.ts#L86-L102
[amp-recstream]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/ampChain.ts#L122-L123
[amp-engine]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/ampEngine.ts#L1-L11
[amp-presets]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/ampPresets.ts#L41-L62
[live]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/app/live/page.tsx#L23-L34
[rec-model]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/recordings.ts#L5-L25
[rec-item]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/recorder/RecordingItem.tsx#L97-L122
[rec-mime]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/recorder.ts#L17-L22
[ab]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/recorder/AbCompare.tsx#L10-L14
[loop]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/loopPedal.ts#L1-L8
[loopmix]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/loopMix.ts
[loop-ui]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/looper/LoopPedal.tsx
[mt-latency]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/multitrackEngine.ts#L1-L8
[mt-mixdown]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/multitrackEngine.ts#L133-L169
[mt-model]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/multitrack.ts#L7-L22
[mt-guard]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/multitrack.ts#L115-L140
[studio-memory]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/studio/Studio.tsx#L35-L37
[wav]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/wav.ts#L12
[input-mono]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/audioEngine.ts#L36-L44
[latency-est]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/audio/audioEngine.ts#L330-L335
[meter]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/InputLevelMeter.tsx#L9-L10
[backup]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/backup.ts#L1-L5
[slowdown]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/components/practice/RiffTrainer.tsx#L62-L63
[instrument]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/instrument.ts
[amp-sanitize]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/src/lib/ampPresets.ts#L124-L151

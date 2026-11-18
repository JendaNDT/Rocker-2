# Rocker 2 — etapy vývoje

**Dokument 04 z 5.** Pořadí prací pro implementující model (Astra nebo Sol). Každá etapa končí **něčím, co lze předvést**, a má **jasnou podmínku dokončení**. Brzy (v etapě E2) se ověří to nejrizikovější: **dohrání kytary přes bicí** a **opětovné otevření uložené skladby**.

Podklady: 01 (funkce a vzhled), 02 (architektura), 03 (zkoušky A, M a H).

## Pravidla pro všechny etapy

**Každá etapa je hotová, až když:**

1. CI na `windows-2025` i `macos` projde: sestavení, všechny testy a snímky obrazovek.
2. Jsou napsané automatické zkoušky A uvedené u etapy a procházejí.
3. V každé etapě připravíme postup pro ruční zkoušky a v `docs/STAV.md` uvedeme „čeká na ruční zkoušku po vydání“. Jenda a hráč provedou všechna měření M, hraní H a další ruční kontroly až po vydání hotové aplikace. Platí to i pro M-01, H-01, M-02 a H-02. Ve všech níže uvedených podmínkách dokončení patří ruční část k ověření po vydání. Vývoj dalších etap a vydání mohou pokračovat po splnění průběžných automatických zkoušek a ostatních vývojových úkolů. [Rozhodnutí o termínu zkoušek](ROZHODNUTI.md#termin-rucnich-zkousek)
4. Rozhraní odpovídá návrhu v dokumentu 01, kapitola 5. Barvy, písma a rozměry jdou jen přes designové tokeny a snímky obrazovek jsou přiložené. **Vzhled je od nuly, bez inspirace původním Rockerem.**
5. Jsou aktualizované soubory `docs/STAV.md` (co funguje), `docs/PREDANI.md` (předávka další relaci) a `docs/ROZHODNUTI.md` (každá odchylka od dokumentů 01–03 a její důvod).
6. Jenda dostal česky psaný **kontrolní seznam pro ruční vyzkoušení** (co stáhnout, co zapojit, co stisknout a co má slyšet a vidět).

**Velikost etap** je jen orientační: S = malá, M = střední, L = velká.

---

## E0 — Kostra, CI a první zvuk (S–M)

**Cíl:** aplikace se sestaví pro Windows i Mac, má nový vzhled a pustí zvuk do sluchátek.

**Postaví se:**

- repozitář podle 02 / 13.1, CMake s JUCE 9.0.5 a Catch2,
- designové tokeny a vlastní `LookAndFeel`, vložené fonty Inter a JetBrains Mono,
- úvodní obrazovka (01 / 5.7) s rozložením. Nehotové volby ukazují „Připravujeme (etapa Ex)“.
- Průvodce Zapojení, kroky 1–3: výběr ovladače a zařízení, zkušební tón, měřiče vstupů ve skutečných dBFS s přebuzením a pojmenování vstupů. **Bez předpokladů o vybavení:** funguje s 1 i mnoha vstupy, s ASIO i bez něj a bez zvukovky jen v náhradním režimu (01 / kap. 7),
- uložení nastavení počítače,
- kostra jádra s falešným zařízením pro testy,
- CI s artefakty: Windows ZIP pro **x64 i ARM64** a PNG snímky obrazovek,
- simulované sestavy pro zkoušku A-24 (1, 2 a 8 vstupů, bez ASIO, bez zvukovky).

**Předvede se:** na Windows stáhnout ZIP z CI a spustit. Úvodní obrazovka v novém vzhledu. Zapojení: vybrat ASIO zvukovky, zkušební tón zazní vlevo, vpravo a v obou. Brnknutí na kytaru rozhýbe měřič vstupu 1 a při vytočeném gainu se objeví PŘEBUZENO. Totéž běží na Macu.

**Podmínka dokončení:** pravidla 1–6. Zkoušky A-17, testy měřičů a zkušebního tónu, kulatá cesta uložení a načtení nastavení. Část zkoušky A-24, která se týká Zapojení, je zelená. Jenda potvrdí tón v obou sluchátkách a reakci měřiče na Windows.

---

## E1 — Kytara do sluchátek (M)

**Cíl:** kamarád hraje přes aparát Rockeru 2 a ví, jaké má zpoždění.

**Postaví se:**

- odposlech vstupu (Se zvukem / Čistě / Vypnuto),
- aparát, základní řetězec: brána se „Změř ticho“, zkreslení se třemi typy a převzorkováním, korekce, reprobox z IR, hlasitost,
- předvolby Čistý, Crunch, Lead a Metal,
- ladička (T) s laděními, vlastními laděními, referenčním tónem a potvrzením jen do sluchátek,
- panel Co slyším (zatím bez stop),
- volba **zdroje kytary** (přímo, kombo s mikrofonem, linka, obojí zároveň) podle 01 / 4.3,
- první verze **Kontroly sestavy** v Diagnostice (ovladač, vstupy, zpoždění, procesor),
- **Diagnostika → Změřit zpoždění** (smyčkový kabel) a ukládání kalibrace,
- varování u Bluetooth.

**Předvede se:** kytara zní ve sluchátkách se zkreslením Crunch. Panel ukazuje „Zpoždění 7 ms – dobré“. Ladička naladí Drop D.

**Podmínka dokončení:** A-16, A-20, **M-01 a H-01 provedené** (výsledek H-01 určí výchozí buffer). Pokud H-01 selže i s ASIO, rozhodnutí podle 03 / Pořadí zkoušek se zapíše do `ROZHODNUTI.md`.

---

## E2 — Bicí, kytara přes ně, uložení a znovuotevření (L) — klíčová etapa

**Cíl:** projít kroky 1–2 z 01 / 2.2 *a druhý den pokračovat*. **Tady se rozhodne, jestli vlastní jádro obstojí** (02 / kap. 2).

**Postaví se:**

- skladba (model 02 / kap. 5) s jedním dílem; stopy (přidat, pojmenovat, druh, vstup, ●, Ztlumit, Sólo),
- tempo, metrum, klik jen do sluchátek a odpočet, ťukání tempa,
- **nahrávání při poslechu stop s kompenzací zpoždění**,
- přehrávání; časová osa s pravítkem taktů, průběhy a ukazatelem,
- stavový pás včetně červeného stavu a rámu při nahrávání a jednoduchý Pohled zdálky (červená plocha, číslo taktu, odpočet),
- automatické ukládání (atomicky), `zobrazeni.json`, úvodní nabídka „Pokračovat“,
- deník záchrany a obnova po pádu, zákaz spánku během nahrávání,
- **Diagnostika → Zkouška souhry** (pro M-02),
- vnitřní offline render bez tlačítka Export, potřebný pro test A-06.

**Předvede se:**

1. Nová skladba, 112 BPM, odpočet, 8 taktů bicích.
2. Kytara při poslechu bicích (zatím přes aparát z E1, ale zvuk aparátu se nahrává *čistý*: odposlech je se zvukem, do souboru jde čistý signál).
3. Zavřít, otevřít a pokračovat na stejném místě.
4. Během nahrávání „Ukončit úlohu“ ve Správci úloh a po startu nahrávku obnovit.

**Podmínka dokončení:** A-03, A-04, A-06, A-07, A-08, A-09, A-14 a A-15 zelené. **M-02 prošlo** se smyčkovým kabelem. **H-02 provedené**, M-04 provedené, H-10 (základ) provedené.

**Pokud M-02 nebo A-07 neprojde ani po opravě:** zastavit, zapsat zjištění do `ROZHODNUTI.md` a s Jendou rozhodnout o přechodu na Tracktion Engine (02 / kap. 2). Rozhraní a DSP zůstávají.

---

## E3 — Zvuk kytary jako nastavení stopy (M)

**Cíl:** „zítra vyzkoušet jiný zvuk na téže nahrávce“.

**Postaví se:**

- aparát jako zvuk stopy při přehrávání, s vyrovnáním latence aparátu,
- úplný řetězec: ozvěna a deterministický dozvuk,
- panel Zvuk kytary ve spodní desce, štítek zvuku v hlavě stopy,
- kopie předvolby do skladby, *Uložit jako předvolbu*, hvězdička u upravené předvolby.

**Předvede se:** nahraná kytara přepnutá z Crunch na Metal a zpět zní hned jinak. Uložit, zavřít, otevřít a zvuk zůstal.

**Podmínka dokončení:** A-05 (pro aparát), A-20, **H-03 provedené**, M-05 (první měření výkonu).

---

## E4 — Úpravy a Zpět (M)

**Cíl:** stříhat přesně a beze strachu.

**Postaví se:**

- režim Stříhej (01 / 5.4): výběr taktů klikem na čísla, rozstřihnout (X), přesun s přichytáváním (Alt = volně), kopie, smazání, zopakovat (D),
- tažení okrajů s lupou, přiblížení a oddálení,
- Zpět a Znovu, historie verzí (*Vrátit skladbu do stavu z…*).

**Předvede se:** vystřihnout takty 5–6, přesunout je na takt 9, zopakovat a vše vrátit Ctrl+Z.

**Podmínka dokončení:** A-01, A-17 (režim Stříhej). Jenda zvládne podle kontrolního seznamu střih přesně na začátek taktu.

---

## E5 — Oprava taktu a pokusy (M)

**Cíl:** krok 3 z 01 / 2.2.

**Postaví se:**

- **P – Oprav takt** s předstihem, dohrou a odpočtem; nahrává se celý průchod,
- skupiny pokusů a štítek Pokusy nad úsekem, **Tab**, **Enter**, *Zahodit pokus* (koš skladby),
- prolnutí 10 ms, přepínání živého vstupu v aparátu,
- panel Pokusy ve spodní desce.

**Předvede se:** chyba v taktu 6, P, Tab, Enter. Tři pokusy a návrat k prvnímu.

**Podmínka dokončení:** A-21, testy modelu pokusů, **H-04 provedené**.

---

## E6 — Díly a pořadí (M)

**Cíl:** krok 4 z 01 / 2.2.

**Postaví se:**

- pás Díly: rozdělit díl, název, barva (pásky), výběr celého dílu,
- **zopakovat díl jako spojený výskyt ⧉**, *Osamostatnit*,
- zkrácení výskytu s posunem zbytku skladby, šrafovaný zbytek, *Obnovit celou délku*,
- dotaz *Opravit oba / Jen tento výskyt* před opravou ve spojeném dílu.

**Předvede se:** Sloka + Refrén + Refrén ⧉, zkrátit druhý refrén na 6 taktů a vrátit všemi třemi způsoby.

**Podmínka dokončení:** A-02 a úloha z kroku 4 projitá podle kontrolního seznamu.

---

## E7 — Mix a export (M)

**Cíl:** krok 5 z 01 / 2.2.

**Postaví se:**

- režim Míchej s **mixem v řádcích** (01 / 5.5): hlasitost, poloha (konstantní výkon, −3 dB ve středu), Sólo a Ztlumit s indikací SÓLO ve stavovém pásu,
- měřiče s rozlišením PŘEBUZENÝ VSTUP a PŘEBUZENÝ MIX a se značkou místa,
- Celkem,
- **Poslat mix** (WAV 24/48, WAV 16/44,1 s ditherem, MP3 320 přes Media Foundation), upozornění na sólo, hlášení přebuzení, *Přehrát výsledný soubor*.

**Předvede se:** smíchat bicí a kytaru, exportovat MP3, pustit v telefonu.

**Podmínka dokončení:** A-05, A-06, A-12, M-03, M-13. Poslech exportu potvrdí, že zní jako při míchání.

---

## E8 — Nápady, Pojistka a import (M)

**Cíl:** cesta 01 / 2.1.

**Postaví se:**

- **Nahrát nápad** s předstihem 2 s, **Pojistka** a **U – Ulož, co jsem hrál**,
- knihovna Nápady (★, poznámka, filtr, náhled),
- *Udělat z toho skladbu*, **Doťukat tempo**,
- import (WAV, AIFF, FLAC, OGG, Opus, MP3; M4A přes kodeky Windows, pokud jsou k dispozici) s převodem frekvence,
- úplná úvodní obrazovka.

**Předvede se:** nápad z gauče, druhý den ★ nápad, z něj skladba a k ní bicí.

**Podmínka dokončení:** A-18, A-19, H-11, H-12.

---

## E9 — Bicí naplno a ovládání s nástrojem v ruce (M)

**Cíl:** všechny sestavy bicích z 01 / 4.4 a ovládání z dálky.

**Postaví se:**

- **rozpoznání sestavy bicích** z výzvy „Zahraj na bicí“ podle počtu vstupů (01 / 4.4), včetně rady, když vstupů není dost,
- **Sada bicích** (více vstupů, společné pokusy a střihy, skupinový jezdec, předvolby poloh),
- stereo elektronické bicí,
- test nejsilnějšího úderu,
- úplný **Pohled zdálky** (doby jako čtverce, „5 / 8“, zelené Uloženo, nabídka při nahrávání bicích),
- numerická klávesnice u bicích a přiřazení nožního spínače.

**Předvede se:** kamarád nahraje bicí z místa za soupravou bez vstávání.

**Podmínka dokončení:** H-05, H-06, H-07, H-08 a část A-24 pro sestavy bicích.

---

## E10 — MIDI bicí, základ (M–L)

**Cíl:** nahrát elektronické bicí, které posílají jen údery, a měnit jejich zvuk po nahrání (01 / 4.4).

**Postaví se:**

- rozpoznání zařízení MIDI v Zapojení a stopa *Bicí (MIDI)*,
- záznam úderů s kompenzací a deníkem pro záchranu, ukládání jako `.mid` (02 / kap. 7, bod 7),
- **vestavěná sada bicích** (přehrávač vzorků, jedna sada se vzorky s licencí CC0 nebo z vlastní nahrávky), výměna sady po nahrání,
- **Naučit pady** a výchozí rozložení podle General MIDI,
- současné nahrávání zvuku modulu a MIDI,
- oprava taktu, pokusy, díly, střih a export i pro stopy MIDI,
- **Diagnostika → Porovnat MIDI a zvuk** (pro M-14).

**Předvede se:** s libovolným MIDI ovladačem nebo modulem bicích: Naučit pady, nahrát 8 taktů přes kytaru, vyměnit sadu a opravit takt.

**Podmínka dokončení:** A-22, A-23 a část A-24 pro MIDI. Pokud má kamarád elektronické bicí, H-13 a případně M-14 provedené. Jinak se ověří s jakýmkoli MIDI zařízením, které má Jenda, a H-13 zůstane jako dluh.

---

## E11 — Odolnost, balení a akceptace v1 (M)

**Cíl:** v1 je hotová.

**Postaví se:**

- hlídání místa a řízené zastavení,
- odpojení a návrat zvukovky,
- varování OneDrive a různá zařízení pro vstup a výstup,
- **Zabalit skladbu / Otevřít balík**, přemapování vstupů na jiném počítači,
- *Uklidit skladbu*,
- instalátor Inno Setup a GitHub Release,
- Diagnostika → *Uložit zprávu*.

**Předvede se:** **celá písnička** (H-09) a přenos skladby na jiný počítač.

**Podmínka dokončení:** A-10, A-11, A-13, celá A-24, M-06 až M-12, **M-15 u kamaráda** a **H-09 a H-10 provedené**.

---

## Po v1

Rozšíření podle 01 / kap. 6, v tomto pořadí: nahrávání dokola (smyčka) → vodicí rytmus → doznívání přes hranu dílu → export stop → MIDI spínač a pad → úpravy jednotlivých úderů MIDI → ostatní.

# Předání E10 · elektronické bicí

## 1. Hotovo

Připojený modul je v Zapojení pod **Bicí (MIDI)**. V nové skladbě zapni záznam, po zastavení otevři nastavení stopy a vyber druhou sadu. Připojením stereo výstupu modulu můžeš stejný výkon zachytit i jako zvuk. [Postup](../README.md#jak-nahrát-elektronické-bicí-přes-midi) · [Zkoušky, balíky a snímky](https://github.com/JendaNDT/Rocker-2/pull/11)

## 2. Zbývá

Poslední etapa zahrnuje přenos a úklid skladby, ochranu při zaplnění disku, diagnostickou zprávu, instalátory a vydání v1.

## 3. Ruční zkoušky

H-13 se skutečným modulem, poslech sad a M-14 se skutečným stereo výstupem modulu **čekají na ruční zkoušku po vydání v1**.

## 4. Rozhodnutí

Obě sady používají původní zvuky s licencí CC0. Přehrávač má pevné hlasy a předem vytvořené vzorky. Zdroj zůstává v MIDI i po změně sady a exportu. [Rozhodnutí E10](ROZHODNUTI.md#e10--původní-sady-a-přehrávání-úderů)

## 5. Prompt pro etapu E11

### Podklady

Ověř stav větví, PR, automatických zkoušek a vydání Rocker-2. Přečti 01–05, STAV, ROZHODNUTI a poslední předání na ověřené větvi. Rozsah určuje dokument 04 / E11, chování programu zejména 01 / 4.8 a architektura 02 / kap. 10–13.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Pracuj jen v Rocker-2. Zachovej offline provoz, C++20, JUCE 9.0.5, české texty pohromadě, kreslení přes tokeny, licence AGPLv3, OFL a CC0. Ve zvukovém vlákně dodrž 02 / 4.3. Zachovej původní zvuk, soubory MIDI, pokusy, společné díly a starší skladby.

### Úkol

Hlídej disk a řízeně zastav s uložením. Obnov zařízení po odpojení. Přidej varování OneDrive a oddělených časových základen ve Windows Audio. Umožni zabalit a rozbalit skladbu, přemapovat vstupy, uklidit nepoužité soubory do systémového koše a uložit zprávu z diagnostiky. Připrav instalátory Inno Setup, přenosné ZIPy a GitHub Release v1.

### Ověření

Spusť A-10, A-11, A-13, celou A-24 a všechny předchozí zkoušky. Prověř sestavení a balíky na Windows x64, Windows ARM64 a Macu a ověř dostupnost stažitelných prostředků vydání. Veškeré hraní, měření a ruční zkoušky na skutečné sestavě následují až po vydání.

### Předání

Použij větev etapa-11, malé commity a český PR. Aktualizuj český README, STAV, PREDANI a ROZHODNUTI. Přidej odkazy na ověřený běh, balíky, instalátory, snímky a vydání v1. Až ověříš vydanou verzi, pozastav automatickou úlohu.

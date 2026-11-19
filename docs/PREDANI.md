# Předání E4 · přesný střih nahrávky

## 1. Hotovo

V otevřené skladbě stiskni **2**. Vyber stopu, klikni na takt **5** a se Shift na **6**. Přesuň vybraný zvuk na takt **9**, zopakuj jej klávesou **D** a vrať oba kroky přes **Ctrl+Z**. Okraje mají lupu ×4 a volný pohyb s Alt. **Soubor → Vrátit skladbu do stavu z…** umožňuje obnovit dřívější skladbu. [Sestavení, ověřené testy a balíky](https://github.com/JendaNDT/Rocker-2/pull/5) · [Výběr](zkousky/E4/strihej.png) · [Lupa](zkousky/E4/lupa.png)

## 2. Zbývá

E4 navazuje na větev E3. Následuje nahrání více pokusů opravy stejného taktu v E5. Přesun na obsazené místo nahrazuje materiál ve vybrané stopě. Předchozí stav lze obnovit přes Zpět.

## 3. Ruční zkoušky

Přesné umístění střihu a poslech přechodu na vlastní nahrávce čeká na ruční zkoušku po vydání. Totéž platí pro poslech a měření z E0–E3. [Postup pro Jendu](../README.md)

## 4. Rozhodnutí

Historie v paměti uchovává všechny kroky otevřené skladby, posledních padesát datovaných verzí zůstává na disku. Úpravy ponechávají původní WAV čitelné. [Rozhodnutí E4](ROZHODNUTI.md#e4--střih-a-verze)

## 5. Prompt pro etapu E5

Jsi hlavní vývojář Rockeru 2. Jenda schvaluje výsledek, zkouší s hráčem po vydání a požádal o samostatné dokončení etap až po E11. Piš česky, srozumitelně pro muzikanta a začni krátkým plánem.

### Podklady

Přečti dokumenty 01–05, STAV.md, ROZHODNUTI.md a toto předání v poslední automaticky ověřené větvi. Pro E5 platí 04 a scénář opravy v 01 / 2.2 a 5.4. Ověř stav otevřených PR.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** C++20, JUCE 9.0.5, vlastní komponenty, designové tokeny, české texty na jednom místě, anglický kód a komentáře. Provoz offline, AGPLv3, licence fontů. Dodrž pravidla zvukového vlákna 02 / 4.3. Zachovej suché zdrojové WAV, nedestruktivní střih, kompenzaci zpoždění a souhru aparátu.

### Úkol

V režimu Stříhej přidej **P – Oprav takt** s předstihem, odpočtem a dohrou. Uchovej celý nahraný průchod. Seskupej pokusy, ukaž jejich štítek nad úsekem a čas vzniku ve spodní desce Pokusy. Tab prochází další pokus, Enter ponechá vybraný, Zahodit pokus přesune do koše skladby. Nastav desetimiliskundové prolnutí na hranách a přepnutí živého vstupu v aparátu. Začni původním zahráním, oprav chybu v taktu 6 dvakrát a vrať první pokus.

### Ověření

Proveď A-21 včetně měření nespojitosti na hranách opravené sinusovky, testy modelu pokusů, znovuotevření, Zpět a předchozí automatické testy. Připrav postup H-04 a označ jej „čeká na ruční zkoušku po vydání“. Ověř sestavení na Windows x64, Windows ARM64 a Macu a ulož snímky.

### Předání

Ve větvi etapa-5 dělej malé commity a otevři český pull request s výsledky, balíky a snímky. Aktualizuj STAV.md, ROZHODNUTI.md a PREDANI.md v pořadí Hotovo, Zbývá, Ruční zkoušky, Rozhodnutí, Prompt pro E6. Pokračuj podle dokumentu 04 bez čekání na potvrzení etapy. Výsledek sděl Jendovi na 5–10 řádků.

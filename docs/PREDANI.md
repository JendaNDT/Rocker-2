# Předání E6 · díly a pořadí

## 1. Hotovo

V režimu Stříhej klikni na pás Díly mezi takty 8 a 9. Rozděl skladbu, zopakuj Refrén klávesou D a přetáhni pravý okraj druhého výskytu o dva takty zpět. První refrén hraje osm taktů, druhý šest. Výběr rozsahu opravy umožňuje změnit oba nebo jen jeden. [Sestavení, běh CI a balíky pro všechny platformy](https://github.com/JendaNDT/Rocker-2/pull/7) · [Díly](zkousky/E6/dily.png) · [Oprava ve spojeném refrénu](zkousky/E6/oprava.png)

## 2. Zbývá

Po E6 navazuje E7 s mixem a exportem. Doznívání přes zkrácený konec dílu je podle návrhu až rozšíření po v1.

## 3. Ruční zkoušky

Poslech hran a průchod hráče krokem 4 z dokumentu 01 čeká na ruční zkoušku po vydání. Totéž platí pro měření na skutečné zvukovce a poslech předchozích etap. [Postup pro hráče](../README.md#jak-zopakovat-refrén)

## 4. Rozhodnutí

Při převodu oprav mezi spojenými výskyty se hranice na dobách počítají v cílovém výskytu. Ořez výskytu používá krátké ztišení a náběh a zachovává zdrojové WAV. [Rozhodnutí E6](ROZHODNUTI.md#e6--díl-a-jeho-výskyty)

## 5. Prompt pro etapu E7

### Podklady

Jsi hlavní vývojář Rockeru 2. Ověř stav větví, PR, skutečných výsledků CI a vydání. Navazuj na poslední ověřenou větev podle dokumentů 01–05, STAV, ROZHODNUTI a tohoto předání. Pro mix a export platí 04 / E7, 01 / 2.2 krok 5, 4.7 a 5.5, 02 / kap. 8 a 03.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** C++20, JUCE 9.0.5 a vlastní kreslení přes tokeny. Texty česky na jednom místě, komentáře a kód anglicky. Provoz offline, AGPLv3 a licence fontů. Dodrž 02 / 4.3. Zachovej čitelnost starých skladeb, suché WAV, kompenzaci zpoždění, pokusy, ořezy, pořadí dílů i Zpět.

### Úkol

Postav režim Míchej. V řádcích nabídni hlasitost, panoráma s konstantním výkonem a středem −3 dB, sólo, ztlumení a viditelný stav SÓLO. Rozliš přebuzený vstup a přebuzený mix, zaznamenej místo a přidej ovládání Celkem. Funkce Poslat mix má upozornit na sólo, najít přebuzení, nabídnout úpravu celkové hlasitosti a přehrát výsledný soubor. Exportuj WAV 24 bit / 48 kHz, WAV 16 bit / 44,1 kHz s ditherem TPDF a MP3 320 kb/s přes Windows Media Foundation. Tam, kde kodér chybí, vysvětli dostupné možnosti. Připrav diagnostiku pro M-03.

### Ověření

Spusť A-05, A-06, A-12 a všechny předchozí zkoušky. Porovnej poslech s exportem včetně dílů a aktivních pokusů. Ověř dekódování a délku MP3, případnou nedostupnost kodéru zaznamenej podle dokumentu 03. Ulož snímky a sestavení pro Windows x64, Windows ARM64 a Mac. M-03, M-13 a poslech v telefonu čekají na ruční zkoušku po vydání.

### Předání

Pracuj ve větvi etapa-7 po malých commitech. Otevři český PR s výsledky a odkazy na balíky a snímky. Aktualizuj STAV, ROZHODNUTI a toto předání v pořadí Hotovo, Zbývá, Ruční zkoušky, Rozhodnutí, Prompt pro E8. Pokračuj podle dokumentu 04 bez čekání na schválení etapy.

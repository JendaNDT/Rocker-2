# Předání E2 · bicí, kytara a návrat ke skladbě

## 1. Hotovo

Nová skladba otevře stopy Bicí a Kytara. Vstup a příprava k nahrávání se nastavují pro každou stopu. R spustí záznam s volitelným odpočtem. Ve sluchátkách může hrát klik, předchozí stopy a živá kytara přes aparát. Každá připravená stopa dostane čistou mono nebo stereo nahrávku. Kliknutím do pravítka lze vybrat místo, od kterého začne další průchod. Pokračovat obnoví skladbu i pohled. [Postup pro hráče](../README.md)

Automatické zkoušky, sestavení pro oba druhy Windows a Mac a soubory ke stažení jsou uvedené u návrhu E2. [Sestavení, výsledky a snímky](https://github.com/JendaNDT/Rocker-2/pull/3) · [Časová osa](zkousky/E2/skladba.png) · [Pohled zdálky](zkousky/E2/zdalky.png)

## 2. Zbývá

Doplnit skutečná měření a hodnocení hráče po vydání. Fyzická souhra, záchrana na kamarádově počítači a konečný výběr výchozího bufferu zůstávají otevřené. Zvuk kytarového aparátu při přehrávání nahrané stopy je úkol E3. [Stav projektu](STAV.md)

## 3. Ruční zkoušky po vydání

| Zkouška | Stav |
|---|---|
| M-02 · skutečná smyčka, deset minut, restart a změna bufferu | čeká na ruční zkoušku po vydání |
| H-02 · osm taktů bicích a kytary, poté pět minut | čeká na ruční zkoušku po vydání |
| M-04 · ukončení skutečného nahrávání ve Správci úloh | čeká na ruční zkoušku po vydání |
| H-10 · pokračování po restartu počítače | čeká na ruční zkoušku po vydání |
| Zvukovka, sluchátka, M-01 a H-01 z předchozích etap | čeká na ruční zkoušku po vydání |

## 4. Rozhodnutí

První díl se podle nahrávky prodlouží. Přeplnění desetisekundové fronty ukončí záznam a místo výpadku dostane značku. Diagnostika měří v dočasné skladbě. [Důvody a alternativy](ROZHODNUTI.md)

## 5. Prompt pro etapu E3

Jsi hlavní vývojář českého domácího studia Rocker 2. Jenda rozhoduje a schvaluje, ty píšeš kód, testy a dokumentaci. Začni krátkým plánem.

### Podklady

Přečti dokumenty 01–05, STAV.md, ROZHODNUTI.md a toto předání. Rozsah E3 určuje 04. Ověř E2, zejména desetiminutový záznam přes přehrávanou stopu a znovuotevření skladby.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Původní repozitář neměň ani neotvírej jeho snímky, navigaci či styly. C++20, JUCE 9.0.5, vlastní zvukové jádro, Catch2, vlastní kreslení JUCE a společné designové tokeny. České texty v jednom místě, anglický kód a komentáře. Provoz offline, AGPLv3 a licence písem. V callbacku platí 02 / 4.3. Nabídka závisí na vybavení. Ruční zkoušky až po vydání hotové aplikace.

### Úkol

Při přehrávání kytarové stopy použij její aparát s kompenzací zpoždění. Zachovej čistý zdroj WAV. Doplň ozvěnu a deterministický dozvuk. Otevři Zvuk kytary ve spodní desce, název zvuku ukaž v hlavě stopy. Parametry předvolby zkopíruj do skladby. Přidej Uložit jako předvolbu a hvězdičku po ruční změně.

### Ověření

Přepni nahranou kytaru z Crunch na Metal a zpět, ulož skladbu a znovu ji otevři. Proveď A-05 pro aparát, A-20 i předchozí testy E0–E2. Ověř shodu zvuku při přehrávání a vnitřním renderu, souhru s bicími a znovuotevření. Sestav Windows x64, ARM64 a Mac a ulož snímky. Pro H-03 připrav poslech čtyř předvoleb a hodnocení použitelnosti. Pro M-05 připrav první měření výkonu. Obě ruční zkoušky označ jako čekající po vydání.

### Předání

Pracuj ve větvi etapa-3 po malých commitech, otevři český pull request s odkazy na sestavení a snímky. Aktualizuj STAV.md, ROZHODNUTI.md a PREDANI.md v pořadí Hotovo, Zbývá, Ruční zkoušky, Rozhodnutí, Prompt pro E4 podle 04. Napiš Jendovi česky na 5–10 řádků.

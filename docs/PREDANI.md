# Předání E3 · zvuk nahrané kytary

## 1. Hotovo

Otevři skladbu se zaznamenanou kytarou, spusť přehrávání a klikni na štítek zvuku u stopy. Ve spodní desce vyber Metal a vrať se na Crunch. Posun jezdce přidá hvězdičku. Přes **Uložit jako předvolbu** si zvuk pojmenuješ i pro další skladby. **Pokračovat** obnoví nastavení jednotlivých stop. Předvolba může z knihovny zmizet a skladba si ponechá všechny její hodnoty.

Návrh E3 odkazuje na ověřený běh pro Windows x64, Windows ARM64 a Mac, tři aplikace i jejich snímky. [Výsledky a soubory ke stažení](https://github.com/JendaNDT/Rocker-2/pull/4) · [Spodní deska Zvuk kytary](zkousky/E3/zvuk-kytary.png) · [Podrobný postup](../README.md)

## 2. Zbývá

Sloučení E2 a navazující E3 do hlavní větve. Kvalitu jednotlivých zvuků a výkon kamarádova notebooku posoudí hráč po vydání. V Diagnostice lze spustit desetiminutovou skladbu s šestnácti stopami a vybrat až osm aparátů. Dostupnost počítadla výpadků závisí na ovladači zvukovky.

## 3. Ruční zkoušky

| Zkouška | Stav |
|---|---|
| H-03 · poslech čtyř předvoleb, přepnutí během hry a návrat do skladby | čeká na ruční zkoušku po vydání |
| M-05 · síť a baterie, ASIO 128, nejvyšší zátěž a výpadky | čeká na ruční zkoušku po vydání |
| Poslech, kabelová měření, hraní a obnova na skutečném vybavení z E0–E2 | čeká na ruční zkoušku po vydání |

## 4. Rozhodnutí

Starší skladba se otevře s původním čistým zvukem. Nová přímo připojená kytara má Crunch. Ozvěna a dozvuk sdílejí nastavení při hraní, přehrávání a renderu. [Podrobnosti a alternativy](ROZHODNUTI.md#e3--zvuk-kytary-ve-skladbě)

## 5. Prompt pro etapu E4

Jsi hlavní vývojář českého domácího studia Rocker 2. Jenda rozhoduje, zkouší po vydání a schvaluje. Ty píšeš kód, testy a dokumentaci.

### Podklady

Přečti dokumenty 01–05, STAV.md, ROZHODNUTI.md a toto předání. Rozsah E4 určuje dokument 04. Začni krátkým plánem a ověř, která větev obsahuje přezkoušenou E3.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Použij vlastní kreslení komponent JUCE a společné tokeny. Původní repozitář neměň. C++20, JUCE 9.0.5, provoz offline, české texty na jednom místě, anglický kód a komentáře, AGPLv3 a licence písem. Zvukové vlákno dodržuje 02 / 4.3. Zachovej čisté WAV, obnovu po pádu i zvuk stop. Ruční zkoušky proběhnou po vydání hotové aplikace.

### Úkol

Vytvoř režim **Stříhej** podle 01 / 5.4. Přidej výběr taktů klikáním na jejich čísla, rozstřižení klávesou X, přesun s přichycením k mřížce a volný přesun s Alt. Podporuj kopii, smazání, zopakování pomocí D, tažení okrajů s lupou a přiblížení. Přidej Zpět, Znovu a návrat k uložené verzi skladby.

### Ověření

Automaticky předveď vystřižení taktů 5–6, přesun na takt 9, zopakování a návrat přes Ctrl+Z. V A-01 proveď 1 000 náhodných úprav a porovnej JSON výchozího i konečného stavu po úplném Zpět a Znovu. Proveď A-17 pro režim Stříhej a znovu všechny testy E0–E3 na obou druzích Windows a na Macu. Připrav pro Jendu postup přesného střihu po vydání.

### Předání

Pracuj ve větvi etapa-4 po malých commitech. Otevři český pull request se snímky a odkazy na ověřené sestavení. Aktualizuj STAV.md, ROZHODNUTI.md a PREDANI.md v pořadí Hotovo, Zbývá, Ruční zkoušky, Rozhodnutí, Prompt pro E5 podle dokumentu 04. Jendovi napiš česky na 5–10 řádků.

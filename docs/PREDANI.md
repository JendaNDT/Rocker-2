# Předání E5 · oprava taktu

## 1. Hotovo

V režimu **2 Stříhej** vyber stopu, klikni na takt 6 a stiskni **P**. Po průchodu se přehrají takty 5–7. Dalším **P** přidej třetí pokus. **Tab** změní pokus na stejném místě přehrávání, **Enter** výběr uloží. Původní zahrání zůstává jako pokus 1. Hranu štítku můžeš přetáhnout do předstihu nebo dohry. [Výsledky sestavení a balíky](https://github.com/JendaNDT/Rocker-2/pull/6) · [Snímek Pokusů](zkousky/E5/pokusy.png)

## 2. Zbývá

Následuje E6. Přidá díly skladby, spojené refrény, zkrácení výskytu a volbu, kde se má oprava projevit. V E5 je oprava přiřazena k místu v jedné stopě. Společné pokusy vícemikrofonové sady bicích patří do E9.

## 3. Ruční zkoušky

H-04 **čeká na ruční zkoušku po vydání**. Jenda s kytaristou ověří dobu potřebnou k opravě, návaznost úderu a dozvuk aparátu. Totéž platí pro dosavadní poslech a měření na skutečné sestavě. [Postup H-04](../README.md#jak-opravit-jediný-takt)

## 4. Rozhodnutí

Koš drží odkaz na celý záznam opravy. Při přerušení uprostřed taktu lze rozehraný průchod obnovit, původní hraní dál zní. [Rozhodnutí E5](ROZHODNUTI.md#e5--záznam-pokusů-a-koš)

## 5. Prompt pro etapu E6

### Podklady

Jsi hlavní vývojář Rockeru 2. Přečti dokumenty 01–05, STAV, ROZHODNUTI a předání z poslední automaticky ověřené větve. Projdi stav větví, PR, výsledky sestavení a vydání. Navazuj na E5. Rozsah určuje 04 / E6 a postup muzikanta 01 / 2.2, krok 4.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Pracuj jen v Rocker-2, C++20, JUCE 9.0.5, vlastní kreslení a tokeny. Rozhraní česky na jednom místě. Aplikace funguje bez připojení, účtu i telemetrie. Zachovej AGPLv3, licence fontů, pravidla zvukového vlákna z 02 / 4.3, čisté zdrojové WAV, historii a čitelnost starších skladeb.

### Úkol

Přidej pás Díly s názvem a barevnou páskou. Umožni rozdělit díl a vybrat jej celý. Opakování vytvoří spojený výskyt se symbolem ⧉. Přidej Osamostatnit, zkrácení výskytu s posunutím následujících dílů, šrafování skrytého konce a Obnovit celou délku. Při opravě spojeného výskytu nabídni Opravit oba / Jen tento výskyt. Do dílů patří také pokusy.

### Ověření

Proveď A-02 a předchozí automatické testy. Vytvoř Sloku a Refrén, zopakuj Refrén, druhý výskyt zkrať na šest taktů, vyzkoušej všechny tři návraty podle 01. Ověř uložení, načtení, zvuk i zásah opravy do správného výskytu. Získej sestavení a snímky z Windows x64, Windows ARM64 a Macu. Poslech a ruční zkoušky jsou až po vydání.

### Předání

Větev etapa-6, malé commity, český PR s výsledky a odkazy na balíky a snímky. Aktualizuj STAV, ROZHODNUTI a PREDANI ve stejném pořadí. Připrav zadání E7 podle dokumentu 04. Po ověření pokračuj bez čekání na Jendovo potvrzení.

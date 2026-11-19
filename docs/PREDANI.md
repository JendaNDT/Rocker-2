# Předání E9 · bicí a ovládání od soupravy

## 1. Hotovo

V Zapojení vyber **Zahraj na bicí**, udeř do bubnů a vyhodnoť vstupy. Sestavu použij v nové nebo rozpracované skladbě. V režimu Míchej vyber jednu stopu sady pro společnou hlasitost **Bicí celkem**. Na numerické klávesnici ovládá záznam Enter, od soupravy je čitelný Pohled zdálky. [Postup pro Jendu](../README.md#jak-připravit-bicí) · [Výsledky sestavení a balíky](https://github.com/JendaNDT/Rocker-2/pull/10)

## 2. Zbývá

Další etapa přidá MIDI bicí, naučení padů, záchranu úderů po pádu a možnost vyměnit sadu ve skladbě.

## 3. Ruční zkoušky

H-05 až H-08 a poslech stereo rozložení **čekají na ruční zkoušku po vydání v1**.

## 4. Rozhodnutí

U dvou aktivních vstupů vybírá hráč mezi stereo modulem a mikrofony. Nastavení se vztahuje ke konkrétní zvukovce. Ve skladbě lze k záznamu použít oba Enter. [Rozhodnutí E9](ROZHODNUTI.md#e9--dvě-aktivní-zásuvky-a-klávesy-u-soupravy)

## 5. Prompt pro etapu E10

### Podklady

Ověř skutečný stav větví, PR, zkoušek a vydání Rocker-2. Přečti dokumenty 01–05, STAV, ROZHODNUTI a poslední předání na ověřené větvi. Rozsah E10 najdeš v 04. Pole pro MIDI a kompenzaci určuje 02 / kap. 5 a 7.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Pracuj jen v Rocker-2. Zachovej češtinu textů rozhraní na jediném místě, tokeny, JUCE 9.0.5, C++20, offline provoz, AGPLv3 a licence přiložených prostředků. Ve zpětném volání platí 02 / 4.3. Zachovej kompatibilitu se staršími skladbami, nápady, původním zvukem a historií.

### Úkol

Přidej skutečná zařízení MIDI, stopu **Bicí (MIDI)**, naučení padů a výchozí General MIDI. Ulož údery s kompenzací a průběžným deníkem do standardního MIDI. Připrav vlastní nebo CC0 vzorky, možnost změnit sadu a současně zachytit stereo zvuk modulu. Zahrň pokusy, opravy, střih, díly, export a diagnostiku **Porovnat MIDI a zvuk**.

### Ověření

Proveď A-22, A-23, odpovídající A-24 a dosavadní automatické zkoušky. Zkontroluj Windows x64, Windows ARM64 a Mac včetně balíků a snímků. H-13, M-14 a poslech s fyzickou soupravou čekají na ruční zkoušku po vydání.

### Předání

Větev etapa-10, malé commity, český PR, snímky, odkazy na běh a balíky. Aktualizuj STAV, ROZHODNUTI a PREDANI v tomto pořadí částí včetně promptu pro E11. Pokračuj samostatně podle 04.

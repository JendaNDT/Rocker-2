# Předání E0

## 1. Hotovo

Místní sestavení a šest automatických zkoušek prošlo. Otevři Zapojení, vyber zařízení, spusť tón a v dalším kroku brnkni. Vstup můžeš pojmenovat podle brnknutí nebo ručně. Aplikace si pamatuje nastavení počítače a jména vstupů.

V návrhu změn otevři podrobnosti příslušné zkoušky. Dole v přehledu běhu najdeš balíky Rocker2-Windows-x64, Rocker2-Windows-ARM64, Rocker2-Mac a ke každému osm snímků. [Výsledky CI](https://github.com/JendaNDT/Rocker-2/pull/1/checks) · [Běhy s artefakty](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml?query=branch%3Aetapa-0) · [Návod pro Jendu](../README.md)

## 2. Zbývá

Podmínkou přijetí E0 je poslech L / P / oba ve sluchátkách a odezva měřiče na skutečném Windows. Výsledek závisí na ovladači, přístupu k mikrofonu a zapojení zvukovky. [Přehled zkoušek a vybavení](STAV.md)

## 3. Ruční zkoušky

| Kde | Výsledek |
|---|---|
| Windows, výběr ASIO, tón, kytara a přebuzení | čeká na Jendu — čeká na ruční zkoušku |
| Mac s Jendovou zvukovkou a CoreAudio | čeká na Jendu — čeká na ruční zkoušku |
| Mikrofon notebooku, volitelně | čeká na Jendu — čeká na ruční zkoušku |
| Náčrty a osm PNG | čeká na Jendu |

Pošli typ počítače, název zvukovky a ovladače, frekvenci, buffer a co jsi slyšel ve třech sekundách tónu. U měřiče stačí číslo vstupu a zda šla značka po ubrání GAIN smazat. Zprávu do záznamu zapíše vývojář.

## 4. Rozhodnutí

Původ dokumentu 05, repozitář, ARM64, zvětšení, prázdný úvod a náhradní zapojení jsou vysvětlené v [ROZHODNUTI.md](ROZHODNUTI.md).

## 5. Prompt pro etapu E1

Jsi hlavní vývojář Rockeru 2. Jenda rozhoduje, hraje, zkouší a schvaluje. Ty píšeš kód, testy a dokumentaci. Jenda nemá upravovat kód.

### Podklady

Přečti celé dokumenty 01–05 v docs, STAV.md, ROZHODNUTI.md a toto předání. Zkontroluj E0 a zapiš výsledky, které Jenda poslal. Rozsah E1 je v dokumentu 04. Začni plánem do deseti bodů.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Původní repozitář neměň. Neotvírej jeho vizuální komponenty, navigaci, styly ani snímky. Používej komponenty JUCE s vlastním kreslením a společnými tokeny. Rozhraní česky na jednom místě, kód a komentáře anglicky. Bez účtu, sítě za běhu a telemetrie. Licence AGPLv3, licence přibalených písem a legálně šiřitelná impulsní odezva reproboxu s doloženým původem. Zachovej C++20, JUCE 9.0.5, Catch2, ASIO, režimy Windows Audio i CoreAudio. Ve zvukovém zpětném volání platí 02 / 4.3. Nabídka odpovídá skutečnému vybavení.

### Úkol E1 — kytara do sluchátek

1. Odposlech vstupu Se zvukem / Čistě / Vypnuto. Panel Co slyším pro živé vstupy a sluchátka, stopy přijdou v E2.
2. Brána se Změř ticho, tři typy zkreslení s převzorkováním, korekce, reprobox s impulsní odezvou a hlasitost. Předvolby Čistý, Crunch, Lead a Metal.
3. Ladička na T, tovární i vlastní ladění podle 01 / 4.3, referenční tón a potvrzení jen do sluchátek. Vyzkoušej Drop B a Drop D.
4. Volba zdroje kytary přímo / kombo s mikrofonem / linka / obojí podle počtu vstupů.
5. První Kontrola sestavy v Diagnostice ukáže ovladač, vstupy, zpoždění a procesor. Změřit zpoždění použije smyčkový kabel a uloží kalibraci pro zařízení, frekvenci a buffer. Bez kabelu ukáže označený odhad. Přidej varování pro Bluetooth.

### Ověření

A-16 prověří přesnost ladičky, A-20 shodu výstupu aparátu po opakované změně předvolby. Zachovej zkoušky E0 a A-17 i PNG nových obrazovek. Sestav a otestuj Windows x64, ARM64 a Mac. Jendovi připrav postup pro kabelové měření M-01 a hraní H-01. Při měření začni s výstupem na minimum. Předveď Crunch ve sluchátkách, údaj zpoždění a naladění Drop D. Naměřenou hodnotu a hodnocení hráče zapiš podle 03. Výchozí buffer určí H-01. Pokud hraní ruší i s ASIO, zapiš rozhodnutí podle 03 / Pořadí zkoušek, včetně možnosti přímého odposlechu.

### Předání

Pracuj ve větvi etapa-1 po malých commitech. Otevři český pull request s odkazy na běh, balíky a snímky. Aktualizuj STAV.md, ROZHODNUTI.md a PREDANI.md ve stejném pořadí s promptem pro E2 podle 04. U každé zkoušky napiš skutečný výsledek. Co vyžaduje zvukovku nebo hráče, označ „čeká na ruční zkoušku“. Poslední zprávu Jendovi napiš na 5–10 řádků.

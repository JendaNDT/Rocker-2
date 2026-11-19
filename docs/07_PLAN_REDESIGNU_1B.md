# Rocker 2 · plán redesignu 1b

Redesign navazuje na ověřenou E18. Podkladem je Jendův ZIP „Redesign aplikace s režimy“, zejména `Rocker Obrazovka B.dc.html`, `Rocker Aplikace B.dc.html` a doprovodná specifikace. Tento plán určuje změny vzhledu vůči kapitole 5 dokumentu 01. Pravidla pro ovládání, ukládání a zvuk platí dál. Maketa používá modelové skladby a zařízení. V aplikaci se zobrazují skutečná data. Rozhraní je v JUCE a funguje offline. Původní Rocker se při práci nepoužívá.

## Pořadí práce

| Krok | Rozsah | Zkouška před pokračováním |
|---|---|---|
| 1. Společný rám a Hraj | Boční lišta 72 bodů, hlavička 52, pravý panel 300 a transport 76. Přenést výběr, připravení, ztlumení, sólo a měření do řádků stop. Zvětšit kontrast názvů, vstupů a taktů. Rozlišit připravení, odpočet a nahrávání. | Při nahrávání, zastavení a přepnutí režimu mají tlačítka přehrávání a záznamu stejné souřadnice. Dlouhý český název a více stop se vejdou při 1280 × 720. |
| 2. Stříhej a Díly | Přesunout výběr, opravu taktů, nahrávání dokola, pokusy, předstih a dohru do pravého panelu. Zachovat práci s díly, historií a zkratkami. | Vybrat takty, nahrát opakování, přepnout pokus pomocí Tab a potvrdit Enterem. Zobrazit pořadí právě nahrávaného pokusu a odlišit jej od případné smyčky přehrávání. |
| 3. Míchej a export | Přesunout fader celku, stereo měřiče, přebuzení a poslední hlasitost do pravého panelu. Zúžit sloupce mixu a přidat sbalení panelu. Zachovat export celku i samostatných stop a všechny dostupné formáty. | Časová osa má v minimálním okně čitelné takty a po sbalení panelu roste. Číselné hodnoty a delší české názvy projdou kontrolou rozložení. |
| 4. Metronom | Přidat samostatnou obrazovku s přehráním kliku, velkým BPM, dobami a ťukáním. V transportu přepínat zapnuto a vypnuto jedním stiskem. V Metronomu volit Klik, Rytmus nebo Bez kliku, hlasitost a zvuk kliku. Použít rytmickou mřížku jádra a nabídnout jen ověřená metra. Změny ve skladbě se uplatní po potvrzení, přehrávací poloha zůstane na odpovídajícím taktu. | Ověřit tempo a přízvuk, režimy, oddělení od záznamu a exportu a návrat do skladby. Volba zvuku a hlasitost dojdou skutečně do sluchátkového výstupu. |
| 5. Úvod, Nápady a pomocné obrazovky | Úvod s Pokračovat, seznamem skladeb, posledními nápady a rychlými akcemi. Ve stejném vizuálním jazyce obsloužit knihovnu Nápadů, nový projekt, otevření balíku, hraní bez skladby a Pohled zdálky. | Otevřít rozehranou skladbu, najít nápad a obnovit nahrávku z Pojistky. Zůstanou dostupné dosavadní funkce a zkratky. |
| 6. Ladička, Zapojení a kontrola | Postavit velkou ladičku a přehled tří kroků Zapojení s pravým panelem. Používat frekvence, velikosti bufferu a vstupy z připojené zvukovky. Zachovat MIDI, nožní spínač, dostupné varianty ovladače, náhradní režim a diagnostiku. | Ověřit sestavy s 1, 2 a 8 vstupy, bez ASIO a při odpojení. Testy rozložení, snímky 1280 × 720 a 1920 × 1080 při 150 %, celé CI a balíky Windows x64, Windows ARM64 a Mac. |

## Společná kontrola

- Používat pojmenované tokeny pro rozměry, barvy a písmo. Přístupná tlačítka mají české názvy a viditelný fokus. Texty patří do `Strings.h`. Inter a JetBrains Mono už jsou vložené. Ikony Lucide se vkládají s licencí.
- Kromě zkoušek jednotlivých obrazovek projít v aplikaci s falešným zařízením celý postup od otevření skladby po export a obnovu. Zkontrolovat verze skladeb vzniklé před redesignem a funkce E12–E18.
- Ke změnám dát do českého PR skutečné výsledky, snímky a balíky. Poslech, měření na fyzické zvukovce a hraní celé písně zůstávají ruční zkouškou po vydání.

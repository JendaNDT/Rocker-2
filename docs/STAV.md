# Stav Rockeru 2

## E3 · změna zvuku po nahrání

Kytarová stopa zpracuje při přehrávání kopii nastavení uloženou ve skladbě. Ve spodní desce lze přepnout aparát, změnit parametry a uložit vlastní předvolbu. Zachování zdrojového WAV, poloha vůči bicím a stejné nastavení po obnovení skladby mají automatické zkoušky. [Snímek](zkousky/E3/zvuk-kytary.png) · [Sestavení a výsledky pro jednotlivé systémy](https://github.com/JendaNDT/Rocker-2/pull/4)

| Zkouška | Co se ověřuje |
|---|---|
| A-05 | Dvě stereo kytary, bicí a doznívající efekty. Reálná cesta přes zvukové jádro s bloky 64, 128, 512 a 1024 se porovnává s WAV v obou kanálech. Tolerance vůči WAV je 0,0000005 plné úrovně, takže rozdíl mezi libovolnými dvěma výsledky může být nejvýše −120 dBFS. Dva soubory z renderu musí být bitově shodné. |
| A-20 | Změna při nepřerušeném přehrávání a návrat na Crunch. Znovuotevření změněného zvuku, vlastní předvolba odstraněná z knihovny, čistý zdroj nahrávky a posun o latenci aparátu. |
| A-17 a provoz jádra | Panel při přehrávání ve dvou velikostech. Na Macu zahrnuje kontrola zvukového vlákna také přehrávání s aktivními aparáty. |
| Příprava M-05 | Automat ověřuje délku, počet stop, osm aparátů a zvuk do posledního bloku. Skutečný běh má v Diagnostice vlastní spuštění a výsledek. |

Na vývojovém Linuxu s procesorem AMD EPYC 9V74 zpracoval Release deset minut této skladby (48 kHz, blok 128, 16 stop, osm aparátů) za 39,75 s, průměrně 6,62 % dostupného času. Šlo o výpočet bez zvukového zařízení. Výpadky ovladače ani provoz na baterii toto měření nepokrývá.

**H-03 i M-05 čekají na ruční zkoušku po vydání.** [Postup pro hráče a měření výkonu](../README.md)

## E2 · nahrávání a návrat ke skladbě

Nová skladba, odpočet a nahrávání jsou dostupné z úvodu. Kontrola v rozhraní prošla cestou od osmi taktů bicích přes samostatnou čistou kytaru až po zavření a opětovné otevření na uloženém vzorku. [Postup pro Jendu](../README.md) · [Výsledky pro jednotlivé systémy](https://github.com/JendaNDT/Rocker-2/pull/3/checks)

| Automatická zkouška | Co se porovnává |
|---|---|
| A-03 | Diakritika, stopy, klipy, převod staršího schématu a uložení pohledu. Při poškozeném popisu se vyhledá čitelná verze. |
| A-04 | Deset minut kliku při 112 BPM. Každý nástup odpovídá přesnému vzorku odvozenému od pořadí doby. |
| A-06 | Dva vyrenderované WAV při různém nastavení kliku a odpočtu jsou bitově shodné. |
| A-07 | Simulovaný vstup má zpoždění 211 vzorků a výstup 173. Návraty impulsů po deseti minutách sedí na původních pozicích. |
| A-08 | Po 20 sekundách je násilně ukončen proces zapisující rostoucí vzorky. Jiný proces ověřuje alespoň 19 sekund souvislého WAV, původní polohu a dokončení obnovy. |
| A-09 | Proces je 1 000krát ukončen při střídavém zápisu dvou stavů. Po každém ukončení musí být skladba kompletní. |
| A-14 | Pětisekundové pozastavení disku zachová všechny vzorky. Při patnáctisekundovém pozastavení je po vyčerpání fronty uložen čas výpadku. |
| A-15 | Na Macu prochází stejná desetiminutová zkouška pod RealtimeSanitizerem a ThreadSanitizerem, včetně ladicího sestavení. Výsledek je v krocích kontroly na GitHubu. |
| A-17 | Snímky založení skladby, dvou nahraných stop a Pohledu zdálky při nahrávání včetně velikosti 1920 × 1080 při 150 %. |

Místně prošly automatické kontroly nahrávání, obnovy a ukládání. **M-02, H-02, M-04 a H-10 čekají na ruční zkoušku po vydání.** Naměřené zpoždění konkrétní zvukovky ani výsledek hraní od Jendy zatím nemáme.

## E1 · kytara do sluchátek

Na Windows x64, Windows ARM64 i macOS prošlo sestavení a všech 12 automatických zkoušek. Každý počítač vytvořil balík s aplikací a 18 snímků. [Ověřený běh](https://github.com/JendaNDT/Rocker-2/actions/runs/44403854014)

| Automatická zkouška | Výsledek |
|---|---|
| A-16 | Při 44,1 / 48 / 96 kHz všechny půltóny od B1 po E4, rozladění +7,5 centu. Čistý tón se vešel do ±1 centu, tón s přidanými harmonickými a šumem do ±3 centů. |
| A-20 | Na stejném vstupu se Crunch po přepnutí na Metal i po dalších předvolbách vrátil k bitově shodnému výstupu při různých velikostech bloků. |
| Směrování | Čistý poslech čte pouze vybraný vstup, oba kanály sluchátek mají stejný signál. Referenční a potvrzovací tón se nepřimíchaly do vstupu. Změř ticho nastavil práh po dvou sekundách. |
| Kabel v simulaci | Pět pokusů na každé ze tří frekvencí našlo známých 373 vzorků i s obrácenou polaritou a útlumem. Bez návratu signálu měření selhalo. Samostatná zkouška přes obrazovku Diagnostika uložila 411 vzorků a korekci +38. Neúspěšný další pokus výsledek zachoval. |
| Ladění a nastavení | Devět továrních ladění, Drop D / B, neplatné názvy tónů, dva různé vstupy a vlastní ladění s českým názvem po novém otevření. |
| A-17 a snímky | Devět obrazovek ve dvou velikostech, včetně ladičky s hrajícím D2. Simulovaná zvukovka dodala signál pro měřiče. |

Při spuštění skutečné aplikace na místním Linuxu bez zvukovky se otevřelo okno 1280 × 720. Klávesa T zobrazila ladičku s radou, kde vybrat vstup. Další stisk vrátil průvodce Zapojení.

**M-01 a H-01 čekají na ruční zkoušku po vydání hotové aplikace.** Čísla ze simulace výše slouží k ověření výpočtu. Jendova zvukovka, sluchátka, procesor, buffer, pět měření a hodnocení riffu zatím čekají na první zápis. Pro hraní ještě nebyl schválen výchozí buffer. [Postup a tabulka pro měření](../README.md) · [Náhled hraní](zkousky/E1/hrani.png) · [Náhled ladičky](zkousky/E1/ladicka.png)

## E0 — první zvuk

Na Windows x64, Windows ARM64 a macOS prošlo sestavení i všech šest automatických zkoušek. GitHub uložil tři balíky s aplikací a tři sady po osmi snímcích. [Ověřený běh CI](https://github.com/JendaNDT/Rocker-2/actions/runs/44374946240)

| Místní zkouška | Výsledek |
|---|---|
| Měřiče | Prošlo. Špička záporného vzorku, ticho, hranice −0,1 dBFS, podržení 1,5 s a ruční zrušení přebuzení zvlášť pro každý vstup. |
| Tón | Prošlo. Každý vzorek na čtyřech výstupech porovnán se sinem 440 Hz, −18 dBFS. Délky, stereo i závěrečné ticho souhlasí při 44,1 / 48 / 96 kHz. |
| Nastavení | Prošlo. Uložení a načtení jmen s diakritikou, zařízení, frekvence a bufferu. Špatný typ, neznámé pole a poškozený JSON aplikaci neshodily. |
| A-17 | Prošlo. Rozměry textu na úvodu, ve třech krocích, u odpovědí na tón i u budoucích funkcí. Vzniklo osm PNG. Fonty dodaly vlastní kresbu ř, ů, ě, ť. |
| A-24 pro Zapojení | Prošlo pro 1, 2, 8 a 18 vstupů při třech frekvencích. Sestavy bez ASIO, mikrofon notebooku, odpojení během průvodce a opětovné připojení. |
| Pojmenování brnknutím | Prošlo. Krátký impuls do druhého vstupu následovaný tichem mezi obnovami obrazovky označil pouze druhý vstup. |
| Spuštění bez zařízení | Na místním Linuxu se otevřelo okno 1280 × 720. Vyzkoušeny tři kroky, návrat na úvod a klávesa T. |

Výsledky cílových počítačů jsou uvedené jednotlivě u návrhu změn. [Výsledky CI](https://github.com/JendaNDT/Rocker-2/pull/1/checks) · [Běhy s balíky a snímky](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml?query=branch%3Aetapa-0)

## Ruční zkoušky E0 po vydání hotové aplikace

- Windows a fyzická zvukovka s ovladačem ASIO nebo Windows Audio. Poslech tří částí tónu, kytara ve vstupu 1, přebuzení, změna bufferu a odpojení USB.
- Mac, CoreAudio a Jendova zvukovka, stejný poslech a práce s gainem.
- Volitelně mikrofon notebooku a varování u jeho skutečného názvu.
- Jendovo porovnání úvodu a Zapojení s 01 / 5.7 a 4.1.

Jenda ověří tón, měřiče i zpoždění na vydané aplikaci. [Kontrolní seznam](../README.md)

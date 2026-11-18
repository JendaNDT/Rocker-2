# Stav Rockeru 2

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

**M-01 a H-01 čekají na ruční zkoušku.** Čísla ze simulace výše slouží k ověření výpočtu. Jendova zvukovka, sluchátka, procesor, buffer, pět měření a hodnocení riffu zatím čekají na první zápis. Pro hraní ještě nebyl schválen výchozí buffer. [Postup a tabulka pro měření](../README.md) · [Náhled hraní](zkousky/E1/hrani.png) · [Náhled ladičky](zkousky/E1/ladicka.png)

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

## Čeká na ruční zkoušku

- Windows a fyzická zvukovka s ovladačem ASIO nebo Windows Audio. Poslech tří částí tónu, kytara ve vstupu 1, přebuzení, změna bufferu a odpojení USB.
- Mac, CoreAudio a Jendova zvukovka, stejný poslech a práce s gainem.
- Volitelně mikrofon notebooku a varování u jeho skutečného názvu.
- Jendovo porovnání úvodu a Zapojení s 01 / 5.7 a 4.1.

Podmínkou přijetí E0 je Jendovo potvrzení tónu a měřiče na Windows. Měření zpoždění patří do E1. [Kontrolní seznam](../README.md)

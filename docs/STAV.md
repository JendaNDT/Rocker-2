# Stav Rockeru 2

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

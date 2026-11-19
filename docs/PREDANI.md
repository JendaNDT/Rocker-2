# Předání Rockeru 2 v1

## 1. Hotovo

Ve skladbě otevři **Soubor → Zabalit skladbu**. Na druhém počítači použij **Otevřít balík** a podle potřeby znovu přiřaď vstupy. Ve Windows vyber instalátor x64 nebo ARM64 podle procesoru. Mac s čipem Apple používá přenosný ZIP. [Stránka vydání a ověření stažení](https://github.com/JendaNDT/Rocker-2/releases/tag/v1.0.0) · [Automatické zkoušky, artefakty a snímky E11](https://github.com/JendaNDT/Rocker-2/pull/12)

[Přiřazení vstupů](zkousky/E11/prirazeni.png) · [Soubor ve skladbě](zkousky/E11/balik.png) · [Diagnostika](zkousky/E11/diagnostika.png)

Automatické zkoušky E11 prošly na všech třech systémech. Na obou typech Windows instalátor spustil aplikaci a odinstalování zachovalo zkušební skladbu. [Protokol zkoušek E11](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081) · [Sestavení a kontrola stažení vydání v1](https://github.com/JendaNDT/Rocker-2/actions/runs/44463135386)

| Počítač | Ověřené sestavení E11 | Obrazovky |
|---|---|---|
| Windows x64 | [Aplikace](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081/artifacts/14500876466) | [Snímky](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081/artifacts/14501126195) |
| Windows ARM64 | [Aplikace](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081/artifacts/14501240454) | [Snímky](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081/artifacts/14501265486) |
| Mac s čipem Apple | [Aplikace](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081/artifacts/14501315606) | [Snímky](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081/artifacts/14500761594) |

## 2. Zbývá

Na prvním skutečném počítači zaznamenat výsledek instalace a přehrát celou písničku od nápadu po hotový mix. Windows aplikace je nepodepsaná. Na Macu je sestavení pro čipy Apple. Kódování MP3 ve Windows závisí na přítomnosti systémového kodéru. WAV lze použít na obou systémech.

## 3. Ruční zkoušky

Veškerý poslech, H-01 až H-13 a měření na fyzických sestavách **čekají na ruční zkoušku po vydání v1**. H-09 zahrnuje celou píseň, H-10 znovuotevření další den, M-15 zprávu o sestavě. Výsledky od Jendy zatím nejsou. [Krátký seznam pro muzikanta](../README.md#co-vyzkoušet-po-vydání) · [Úplný plán zkoušek](03_ZKOUSKY_NA_WINDOWS.md)

## 4. Rozhodnutí

Velké projekty se přenášejí postupně v ZIP64. Historii a dosud použitelné pokusy chrání také úklid do koše. Instalace nevyžaduje správce. Vydání a stav automatických zkoušek se potvrzují podle GitHubu. [Rozhodnutí E11](ROZHODNUTI.md#e11--balíky-koš-a-vydání)

## 5. Další relace po vydání

Nejdříve zkontroluj vydání v1.0.0, přiložené soubory a předání. Po Jendových ručních zkouškách zapiš sestavu a rozliš, co skutečně slyšel, co změřil a co zůstává otevřené. Chyby reprodukuj pomocí existujících automatických zkoušek. Rozšíření po v1 jsou v dokumentu 01 / kap. 6 a 04. Jejich rozsah vybere Jenda.

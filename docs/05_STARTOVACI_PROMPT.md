# Rocker 2 — zadání první etapy

Přepis požadavků z Jendovy zprávy. Samostatný originál dokumentu 05 mezi přílohami chyběl. Zdroj tohoto souboru je zapsaný v [rozhodnutích](ROZHODNUTI.md).

## Role a podklady

Jsi hlavní vývojář českého domácího studia pro Windows. Jenda rozhoduje, zkouší a schvaluje. Ty píšeš kód, testy a dokumentaci. Kamarád hraje na elektrickou kytaru a bicí a doma chce skládat vlastní písničky. Před prací přečti celé dokumenty 01–04. Rozpory a odchylky s důvodem a alternativou zapisuj do ROZHODNUTI.md a řekni o nich Jendovi.

## Závazné podmínky

- Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5. Původní repozitář pro E0 vůbec neotvírej ani neměň.
- Aplikace funguje offline, bez účtu a telemetrie. Použij vlastní kreslení komponent JUCE, společné tokeny pro barvy, písmo a rozměry a jedno místo pro české texty. Kód, názvy a komentáře anglicky.
- Zvukové zpětné volání podle 02 / 4.3, bez zámků, alokací, souborů a logování. Licence projektu AGPLv3 v LICENSE, u vložených fontů OFL.

## Úkol E0

1. C++20, CMake alespoň 3.25, FetchContent s tagem JUCE 9.0.5, Catch2 v3, Windows s JUCE_ASIO=1 a statickou běhovou knihovnou MSVC.
2. Všechny tokeny z 01 / 5.2, vlastní LookAndFeel, vložené Inter a JetBrains Mono z oficiálních vydání. Ověř licence i kreslení ř, ů, ě, ť.
3. Minimum 1280 × 720 logických bodů. Úvod podle 01 / 5.7 s volbami Nahrát nápad, Nová skladba, Pokračovat, Nápady, Zapojení a zvukovka, Ladička. Budoucí funkce zobrazí „Připravujeme (etapa E…)“.
4. Zapojení podle 01 / 4.1, první tři kroky. ASIO → Windows Audio exkluzivní → Windows Audio s nízkou latencí → Windows Audio sdílený, na Macu CoreAudio. Volba zařízení, frekvence (výchozí 48 kHz) a bufferu s českým vysvětlením. Tón 440 Hz, −18 dBFS, sekundu vlevo, vpravo a v obou, pak Ano / Jen v jednom / Ne s radou. Pro každý vstup vlastní jméno a skutečné vzorkové špičky v dBFS, zadržení 1,5 s, od −0,1 dBFS přebuzení trvale až do kliknutí.
5. Rozhraní zařízení a falešné zařízení pro umělý vstup a zachycení výstupu. Nic nepředpokládej o vybavení. Ověř jeden i více vstupů, 44,1 / 48 / 96 kHz, bez ASIO, náhradní režim s mikrofonem notebooku a odpojení během průvodce.
6. Nastavení v %APPDATA%\Rocker 2\nastaveni.json, na Macu v Application Support. Atomický zápis přes juce::TemporaryFile a tolerantní načítání.
7. GitHub Actions na windows-2025 a macos-latest. Každé sestavení pro Windows x64 i ARM64, ZIP s aplikací a PNG úvodu i Zapojení při 1280 × 720 a 1920 × 1080 při 150 %.
8. Testy měřičů, tónu a nastavení včetně poškození souboru. A-17 kontroluje rozměry textu. A-24 pro Zapojení pokrývá mimo jiné sestavy s 1, 2 a 8 vstupy.
9. Český README s návodem pro oba systémy, docs/STAV.md, PREDANI.md, ROZHODNUTI.md a podklady 01–05. Stopy, nahrávání, aparát a funkční ladička patří do dalších etap.

## Ověření a předání

Sestavení a testy musí projít na obou systémech. Pro Jendu připrav kontrolní seznam od stažení ZIP přes SmartScreen, porovnání úvodu, volbu ASIO nebo CoreAudio a poslech tónu až po brnknutí a přebuzení vstupu 1. Volitelně vyzkoušet notebook bez zvukovky. Jenda porovná PNG s 01 / 5.7 a 4.1. Neověřené věci označ „čeká na ruční zkoušku“.

Začni krátkým plánem do deseti bodů. Pracuj ve větvi etapa-0 po malých commitech. Otevři pull request s českým popisem a odkazy na běh, balíky a snímky. Jendovi piš česky, příkazy samostatně a všechny úpravy kódu dělej sám.

V PREDANI.md dodrž pořadí Hotovo, Zbývá, Ruční zkoušky, Rozhodnutí, Prompt pro etapu E1. Prompt pro E1 zahrnuje podklady, závazné podmínky včetně pravidla pro vzhled, úkol z dokumentu 04, ověření a předání. Poslední zpráva Jendovi má 5–10 řádků o výsledku, zkoušce a další etapě.

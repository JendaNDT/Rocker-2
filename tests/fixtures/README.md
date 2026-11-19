# Importní zkouška A-19

Soubor `a19-tone.mp3` obsahuje uměle vytvořený sinus 440 Hz. Má 41 rámců MPEG-1 po 1 152 vzorcích, jeden kanál a frekvenci 44,1 kHz. Zdrojový signál i tento zkušební soubor uvolňujeme pod [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). Soubor se používá jen při zkouškách.

```sh
ffmpeg -f lavfi -i sine=frequency=440:sample_rate=44100:duration=2 -frames:a 40 -c:a libmp3lame -b:a 128k -write_xing 0 -id3v2_version 0 a19-tone.mp3
```

Soubor `a19-tone.m4a` obsahuje jednu sekundu syntetického signálu. Vlevo je sinus 440 Hz a vpravo 660 Hz při 44,1 kHz. Zdroj i vzorek jsou pod CC0 1.0.

```bash
ffmpeg -f lavfi -i 'aevalsrc=0.3*sin(2*PI*440*t)|0.3*sin(2*PI*660*t):s=44100:d=1' -c:a aac -b:a 192k a19-tone.m4a
```

# Importní zkouška A-19

Soubor `a19-tone.mp3` obsahuje uměle vytvořený sinus 440 Hz. Má 41 rámců MPEG-1 po 1 152 vzorcích, jeden kanál a frekvenci 44,1 kHz. Zdrojový signál i tento zkušební soubor uvolňujeme pod [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). Soubor se používá jen při zkouškách.

```sh
ffmpeg -f lavfi -i sine=frequency=440:sample_rate=44100:duration=2 -frames:a 40 -c:a libmp3lame -b:a 128k -write_xing 0 -id3v2_version 0 a19-tone.mp3
```

## Скриптинг при подключении через USB

Используется скрипт, который выводит команды с задержкой. Вывод скрипта передаётся на вход терминальной программы.

```bash
$ cat cmdseq.sh
freq="158 MHz"

printf "\n"
sleep 5
printf "dbg_level 16383 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 15360 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 14336 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 13312 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 12288 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 11264 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 10240 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 9216 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 8192 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 7168 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 6144 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 5120 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 4096 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 3072 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 2048 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 1024 127\n"
printf "test_tone $freq\n"
sleep 1
printf "dbg_level 2 127\n"
printf "test_tone $freq\n"
sleep 1
printf "rfkill\n"
sleep 1

$ bash cmdseq.sh | tio -m INLCRNL /dev/ttyACM0
```
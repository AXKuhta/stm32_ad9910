```bash
sudo apt install make gcc-arm-none-eabi openocd tio
make -j4
make flash
tio /dev/ttyACM0
```

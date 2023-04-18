```bash
sudo apt install make gcc-arm-none-eabi openocd tio
make -j4
make flash
sudo tio /dev/ttyACM0
```

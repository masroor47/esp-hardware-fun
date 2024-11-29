So to work on this, here's what I have to do:

Activate the esp-idf environemnt or something?

```zsh
cd ~/esp/esp-idf
. ./export.sh
```

Cd to this project:

```zsh
cd cd ~/esp/esp_led_project/blink
```

Then build and flash the goddamn program (assuming the esp is at usbserial-10, otherwise check with `ls /dev/cu.*`):

```zsh
idf.py build
idf.py -p /dev/cu.usbserial-10 flash
```

Can monitor with:

```
idf.py monitor
```

And, most importantly, to exit the monitor, gotta press `Ctrl-]`

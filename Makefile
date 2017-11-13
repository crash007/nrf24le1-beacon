COMPORT=/dev/ttyUSB0
SERIAL_MONITOR=gnome-terminal -e 'minicom --device $(COMPORT)'

CCFLAGS =-I ./include -L ./lib/* --std-sdcc99 --model-large --code-loc 0x0000 --code-size 0x4000 --xram-loc 0x0000 --xram-size 0x400

main: main.c
	sdcc $(CCFLAGS) main.c ds18b20.rel crc8.rel
	packihx main.ihx > main.hex
	./programmer.pl main.hex $(COMPORT)
#	$(SERIAL_MONITOR)
	rm -f *.asm *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym *.lnk *.hex
clean:
	rm -f *.asm *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym *.lnk *.hex

dht: dht.c
	#sdcc $(CCFLAGS) dht.c
	sdcc -c --model-large --std-sdcc99 -Iinclude/ dht.c

ds18b20: ds18b20.c
        #sdcc $(CCFLAGS) dht.c
	sdcc -c --model-large --std-sdcc99 -Iinclude/ ds18b20.c

crc8: crc8.c
        #sdcc $(CCFLAGS) dht.c
	sdcc -c --model-large --std-sdcc99 -Iinclude/ crc8.c

eddy: eddystone-url.c
	sdcc $(CCFLAGS) eddystone-url.c
	packihx eddystone-url.ihx > eddystone-url.hex
	./programmer.pl eddystone-url.hex $(COMPORT)
	#       $(SERIAL_MONITOR)
	#rm -f *.asm *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym *.lnk *.hex


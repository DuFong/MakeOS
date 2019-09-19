all: VirtualOS.bin DatePrint.bin

VirtualOS.bin: VirtualOS.asm
	nasm -o VirtualOS.bin VirtualOS.asm

DatePrint.bin: DatePrint.asm
	nasm -o DatePrint.bin DatePrint.asm
	
clean:
	rm -f VirtualOS.bin
	rm -f DatePrint.bin

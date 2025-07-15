## Play and Listen FM via Lora (sx127x) transceiver 

## Прослушивание и вещание в модуляции FM через Lora-трансивер (sx1276/sx1278) и Serial-порт.

Зависимости: pyserial

### Использование:

Подправить pins.h и platformio.ini  для своей платы, собрать и залить прошивку.
Использовать lorafm.py для приема и передачи  

RX:

python3 lorafm.py --read --freq 432800000 --output test.bin

python3 lorafm.py --read --duration=0 --rate=16000 --freq 432800000 | aplay --rate=16000 --channel=1 --format=U8

python3 lorafm.py --read --duration=0 --rate=16000 --freq 432800000 | sox -t raw -c1 -b8 -r 16000 -e unsigned-integer - record.wav


TX:

python3 lorafm.py --write  --freq 432720000 --input test2.bin

sox count.wav -t raw -b8 -e signed-integer -c 1 -r 8000 - |  python3 lorafm.py --write  --freq 432720000 


Примечания:
- Serial через USBCDC (на esp32s2) не вывозит rate 48000гц, нужно пробовать hardware uart 
- Для аппаратного serial видимо нужно убрать строчку   Serial.setRxBufferSize(BUFFERSIZE);
- Есть баги в буферизации 






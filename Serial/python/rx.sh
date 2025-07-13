#python3 lorafm.py --read --freq 432800000 --duration=80000 --output test.bin
python3 lorafm.py --read --duration=0 --rate=16000 --freq 432800000 | aplay --rate=16000 --channel=1 --format=u8

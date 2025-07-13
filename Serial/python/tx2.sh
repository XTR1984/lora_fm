sox count.wav -t raw -b8 -e signed-integer -c 1 -r 8000 - |  python3 lorafm.py --write  --freq 432720000 --txpower 20

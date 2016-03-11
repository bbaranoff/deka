./paplon.py > paplon.log &

sleep 1

./delta_client.py > delta_client1.log &
./delta_client.py > delta_client2.log &
./delta_client.py > delta_client3.log &
# XXX

PYOPENCL_CTX=0 ./oclvankus.py > oclvankus1_1.log &
PYOPENCL_CTX=1 ./oclvankus.py > oclvankus2_1.log &
PYOPENCL_CTX=2 ./oclvankus.py > oclvankus3_1.log &


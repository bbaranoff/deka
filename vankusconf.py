# tables we have
# yeah it's in this silly order
mytables = [132, 140, 108, 116, 100, 124, 132, 140, 108, 116, 100, 124, 132, 140, 108, 116, 100, 124, 132, 140, 108, 116, 100, 124, 132, 140, 108, 116, 100, 124, 132, 140, 108, 116, 100, 124, 132, 140, 108, 116]

# server host and port
HOST, PORT = "localhost", 6666

# how many kernels to run in parallel
kernels = 4095
# XXX 4095
# slices per kernel
slices = 32

# dump computed bursts to files for later analysis - useful for bug hunting
DEBUGDUMP = True

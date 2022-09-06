# tables we have
# yeah it's in this silly order
mytables = [132, 260, 172, 388, 140, 148, 324, 156,364, 164, 220, 356, 412, 428, 500, 436, 180, 188,492, 196, 204, 292, 268, 332, 372, 212, 420,348, 396, 100, 230, 340, 124, 108, 238,116, 404, 250, 380, 276]

# server host and port
HOST, PORT = "localhost", 6666

# how many kernels to run in parallel
kernels = 8191
# XXX 4095
# slices per kernel
slices = 32

# dump computed bursts to files for later analysis - useful for bug hunting
DEBUGDUMP = False

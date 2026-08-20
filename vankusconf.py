# tables we have
# yeah it's in this silly order
mytables = [108, 132, 100, 116, 124, 172, 148, 156, 164, 140, 220, 250, 196, 204, 260, 180, 188, 212, 230, 238, 324, 276, 268, 332, 292, 348, 340, 372, 356, 364, 388, 404, 396, 412, 380, 420, 436, 428, 492, 500]

# server host and port
HOST, PORT = "localhost", 6666

# how many kernels to run in parallel
kernels = 4095

# XXX 4095
# slices per kernel
slices = 64

# dump computed bursts to files for later analysis - useful for bug hunting
DEBUGDUMP = False

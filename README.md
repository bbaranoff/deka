# deka
```bash
dd if=/dev/random of=/mnt1/test.idx bs=8 count=1
wget https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh
bash Miniforge3-Linux-x86_64.sh
source ~/.bashrc
conda create -n py311 python=311
echo "conda activate py311" >> ~/.bashrc
source ~/.bashrc
conda install opencl
conda install pocl
cd deka
nano delta_config.h
```
put kraken offsets in delta_config.h
put index path in delta_config.h
put offsets path in delta_config.h
and ... the magic :
put first offset in delta_binary.h
and size of the first index (corresponding to the first table ls -al) divided by 2048
like this
```
int mBlockIndex[40][10228856+100000];
uint64_t mPrimaryIndex[40][39756+1000];
```
with 10228856 first offset
and 39756 idx length / 2048 corresponding to 100.idx offset 0 in my case
you may have to install cuda or libglibc-dev ...

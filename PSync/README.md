PSync
=============

### Prerequisites

* NS-3 (branch ndnSIM-ns-3.29)
* ndnSIM 2.7 (tagged as ndnSIM-2.7)

### Apply patches

After checking out the correct versions, you have to apply these patches to ndnSIM and NFD:

- `ndnSim_patches/ndnSIM.patch` (to `ns3/src/ndnSIM`)
- `ndnSim_patches/NFD.patch` (to `ns3/src/ndnSIM/NFD`)
- `ndnSim_patches/ndn-cxx.patch` (to `ns3/src/ndnSIM/ndn-cxx`)

These patches are used to implement some additional features that ns-3 doesn't currently support (e.g. loss rate in Wi-Fi networks).

### Compile & Run

```bash
# Clone ndnSim 2.7
mkdir ~/ndnSIM_PSync
cd ~/ndnSIM_PSync
git clone https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
git clone -b ndnSIM-2.7 --recursive https://github.com/named-data-ndnSIM/ndnSIM ns-3/src/ndnSIM

# (Apply patches)

# Build and install NS-3 and ndnSIM
cd ~/ndnSIM_PSync/ns-3
./waf configure -d optimized && ./waf && sudo ./waf install

# Compile and run PSync
# (Move PSyhc/ to ndnSIM_PSync/ directory)
./waf configure && ./run_batch.sh
```

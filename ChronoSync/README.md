ChronoSync
=============

### Prerequisites

* NS-3 (branch `ndnSIM-v2.5`)
* ndnSIM 2.6 (tagged as `ndnSIM-2.6`)
  * Note that you have to remove all `DummyIOService` in NFD 0.6.2 and ndn-cxx 0.6.2.

### Apply Patches

After checking out the correct versions, you have to apply these patches to ndnSIM and NFD:

* `ndnSim_patches/ndnSIM.patch` (to `ns3/src/ndnSIM`)
* `ndnSim_patches/NFD.patch` (to `ns3/src/ndnSIM/NFD`)
* `ndnSim_patches/ndn-cxx.patch` (to `ns3/src/ndnSIM/ndn-cxx`)

These patches are used to implement some additional features that ns-3 doesn't currently support (e.g. loss rate in Wi-Fi networks).

### Compile & Run

```bash
# Clone ndnSim 2.6
mkdir ~/ndnSIM_ChronoSync
cd ndnSim_ChronoSync
git clone -b ndnSIM-v2.5 https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
git clone ndnSIM-2.6 --recursive https://github.com/named-data-ndnSIM/ndnSIM ns-3/src/ndnSIM

# (Apply patches)

# Build and install NS-3 and ndnSIM
cd ~/ndnSIM_ChronoSync/ns-3
/waf configure -d optimized && ./waf && sudo ./waf install

# Compile and run ChronoSync
# (Move ChronoSyhc/ to ndnSIM_ChronoSync/ directory)
./waf configure && ./run_batch.sh
```

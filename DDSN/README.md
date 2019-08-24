DDSN
=============

### Prerequisites

* [ndnSim 2.3](https://ndnsim.net/2.3/)

### Compile & Run

```bash
# Clone ndnSim 2.3
mkdir ~/ndnSIM_DDSN
cd ndnSIM_DDSN
git clone -b ndnSIM-v2 https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
git clone https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
git clone -b ndnSIM-2.3 --recursive https://github.com/named-data-ndnSIM/ndnSIM ns-3/src/ndnSIM
# (Replace changed files, see instructions below)

# Build and install NS-3 and ndnSIM
cd ~/ndnSIM_DDSN/ns-3
./waf configure -d optimized && ./waf && ./waf install

# Compile and run DDSN
# (Move DDSN/ to ndnSim_DDSN/ directory)
cd ~/ndnSIM_DDSN/DDSN
./waf configure && ./myrun.sh
```

### Note

To run the simulations in Wi-Fi, you need to change four files in ns-3/src/ndnSIM. Do the following steps:
1. Replace local 'ns-3/src/ndnSIM/helper/ndn-fib-helper.hpp' with 'changed_ndnSIM_files/ndn-fib-helper.hpp'
2. Replace local 'ns-3/src/ndnSIM/helper/ndn-fib-helper.cpp' with 'changed_ndnSIM_files/ndn-fib-helper.cpp'
3. Replace local 'ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.hpp' with 'changed_ndnSIM_files/forwarder.hpp'
4. Replace local 'ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.cpp' with 'changed_ndnSIM_files/forwarder.cpp'


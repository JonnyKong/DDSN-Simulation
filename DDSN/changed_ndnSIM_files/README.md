## Changed Files

This directory include the changed source files of NS3. Files in these directories have been changed:
1. In `ns-3/src/ndnSim/ndn-cxx/src`:
    * `face.cpp` & `face.hpp`
        * Added API for adding an entry to PIT
2. In `ns-3/src/ndnSim/ndn-cxx/src/detail`:
    * `face-impl.hpp`
        * Added logic for adding an entry to PIT without actually sending the packet
3. In `ns-3/src/ndnSim/NFD/daemon/fw`:
    * `forwarder.cpp` & `forwarder.hpp`
        * Added logic to enable setting loss rates
        * Added callbacks for retrieving statistics
4. In `ns-3/src/ndnSim/helper`:
    * `ndn-fib-helper.cpp` & `ndn-fib-helper.hpp`
    * `ndn-stack-helper.cpp` & `ndn-stack-helper.hpp`
        * Added API for setting loss rate
5. In `ns-3/src/ndnSim/model`:
    * `ndn-l3-protocol.hpp`
6. In `ns-3/src/ndnSim/NFD/daemon/table`:
    * `pit-entry.cpp` & `pit-entry.hpp`
    * `pit-face-record.cpp` & `pit-face-record.hpp`
    * `pit-in-record.cpp` & `pit-in-record.hpp`
    * `sd.cpp` & `sd.hpp`
    * `sd-entry.hpp`
    * `vst.cpp` & `vst.hpp`
    * `vst-entry.hpp`
Release Notes
=============

## Version 0.5.1

- Fix forceful shutdown of `Face` when destructing `Logic` instance
  (Issue #4525)

- Fix compilation against the latest version of ndn-cxx library

- Upgrade `waf` to the latest version and other improvements

## Version 0.5.0

- **Breaking change** Use bzip2 compression of sync Data payload (Issue #4140)

- Disallow Interest loopback on sync prefix (Issue #3979)

- Avoid ABI differences between debug/optimized modes (Issue #4496)

- Extend Socket and Logic API:

   - Allow customization of sync interest lifetime (Issue #4490)

   - Limit the size of created sync Data and enable ability to
     customize the maximum packet size through environment variable
     (Issue #4140)

   - Allow override of the session number

- Disable use of Exclude filter (preparation for Exclude deprecation
  in NDN and implementation was only partially correct)

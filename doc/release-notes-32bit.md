Compatibility
-------------

Bitcoin Core now requires a 64-bit target for hosted builds. ARM32 release
binaries are no longer provided. Bare-metal consensus-library builds remain
available for 32-bit targets.

CI coverage for the remaining 32-bit bare-metal consensus-library target is
limited to compilation and linking. Its tests are not executed.

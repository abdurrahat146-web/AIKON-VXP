AIKON-VXP, chunk 1: bare plain-HTTP networking test
=====================================================

What's in here
---------------
net_test.c    - the app. Opens a plain TCP connection to a server,
                sends one HTTP/1.1 GET, shows whatever text comes back
                on screen. No TLS yet - that's deliberate, so we prove
                the basic path first.
manifest.json - app metadata used when packaging the .vxp. The "imsi"
                field is a 15-zero placeholder right now, because there
                is no SIM in the Nokia yet.
NetTest.vxp   - the built app, packaged with that placeholder IMSI.
                It will NOT launch on the Nokia 210 as-is - Series 30+
                refuses to start a .vxp whose IMSI tag doesn't match
                the SIM physically inside the phone at launch time.

Before you can actually run this on the phone
-----------------------------------------------
1. Edit net_test.c: set SERVER_HOST / SERVER_PORT / SERVER_PATH to a
   real plain-HTTP server you control that returns some text (even a
   one-line "pong" page is enough for this test).
2. Get whichever SIM will end up in the Nokia, read its IMSI (Android:
   Settings > About phone > SIM status, on some versions; dial
   *#06# shows the IMEI, not IMSI - IMSI needs the SIM status screen).
3. Put that IMSI in manifest.json's "imsi" field.
4. Rebuild (see below) - the compile/link steps don't need to be
   redone, only the two packaging steps, so this is fast.

How it was built (fully offline, no arm-none-eabi-gcc needed)
-----------------------------------------------------------
This session has no network access for package installs, so the usual
arm-none-eabi-gcc toolchain couldn't be installed. Instead this used
clang's built-in bare-metal ARM target (-target arm-none-eabi) plus
llvm's own linker (lld) and GNU objcopy, linked straight against the
real MediaTek MRE SDK's precompiled libraries from the mre-makefile
project (github.com/gtrxAC/mre-makefile) - same libraries, same
armv5te target, same linker script, just a different front-end
compiler. A couple of tiny stub headers were needed (freestanding-shim/
stdio.h) because there's no newlib for this target here; they only
supply the FILE/size_t names the SDK headers expect and are never
actually used at runtime.

To rebuild yourself (e.g. with a real arm-none-eabi-gcc toolchain,
which will need no stub headers at all):
  1. Clone https://github.com/gtrxAC/mre-makefile
  2. Drop net_test.c into its src/ folder (remove or rename test.c)
  3. Replace its manifest.json with this one (with your real IMSI)
  4. Run `make`

What was NOT done
------------------
- Not tested on real hardware - no battery or SIM available yet.
- No code-signing/IMSI step beyond the placeholder - that's the part
  that needs your actual SIM, which only you can provide.
- No TLS - chunk 2, after this plain version proves the phone can
  reach the server at all.

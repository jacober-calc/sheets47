# Local changes

`lib/inc/sys_string/impl/platform.h`: permit an explicitly selected
`SYS_STRING_USE_GENERIC=1` on an otherwise unidentified platform. Upstream already
selects the portable generic string implementation above its platform dispatch,
but unconditionally rejects bare-metal targets at the final branch. The generic
storage uses standard C++ facilities; no Unix platform is impersonated and no
new OS services are provided. The device build defines this option explicitly.

The original file checksum remains in SNAPSHOT.json. This is the only source
patch in this helper; all original license and copyright text remains intact.

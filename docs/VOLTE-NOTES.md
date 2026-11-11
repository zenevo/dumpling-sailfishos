# VoLTE investigation notes (2026-11-10)

Status: **not working yet.** This file records what was ruled out, so the next
attempt does not repeat the same dead ends. Nothing here changes the image.

## What the port does today

- The modem attaches, LTE data works, and the port provisions an `ims` APN.
- IMS daemons (`imsqmidaemon`, `imsdatadaemon`, `imsrcsd`, `ims_rtp_daemon`) run.
- `ofono` is the Sailfish fork `1.29+git13.1`, and it **does** contain the IMS
  core: `org.ofono.IpMultimediaSystem`, `ofono_ims_create/register/driver_register`.
- The active telephony chain is the legacy one:
  `ofono → ofono-ril-plugin → ofono-ril-binder-plugin → Android IRadio`.
  `ofono-binder-plugin`, `libofonobinderpluginext` and
  `ofono-binder-plugin-ext-qti` are **not** installed.

## What is *not* the problem

The earlier conclusion "Android 9 has no IMS HAL, so VoLTE is impossible" is
wrong:

- LineageOS 16 `android_device_oneplus_msm8998-common` (blobs from OxygenOS
  9.0.9) declares `vendor.qti.hardware.radio.ims@1.0::IImsRadio/imsradio0,1`.
- On the device, `lshal -i` lists **both IMS 1.0 services as registered**
  (`hwbinder`, owned by `rild` processes).
- `ofonod` itself exports the IMS API — it is missing a driver/backend, not the
  API surface.

## What actually blocks it

1. **Wrong backend.** The QTI extension is written for `ofono-binder-plugin`
   (direct binder), not for the `ofono-ril-*` chain that the device uses. Adding
   only the extension explains the historical "no IMS interface appears".
2. **Only IMS 1.0 is exposed.** The tested QTI extension implementations
   (piggz, VerdandiTeam, UBports) look up 1.0/1.1/1.2 with different fallbacks,
   but their hangup and SMS paths still use the 1.2 HIDL transaction numbers and
   structures. A pure service-lookup fallback is not enough.
3. **SIM visibility.** At the time of the last read-only capture both slots
   reported `SimManager.Present=false` and `IpMultimediaSystem.GetProperties()`
   answered `UnknownMethod`. IMS cannot be registered without a detected SIM,
   so this has to be cleared first.

## Candidate next steps

1. Confirm SIM detection (inserted card, slot) before anything else.
2. Build a test image whose device pattern requires
   `ofono-binder-plugin` + `libofonobinderpluginext` + a QTI extension, with a
   `binder.d` drop-in using `extPlugin=qti` and `radioInterface=1.1`.
3. Adapt the extension for IMS 1.0 (callback interface, CallInfo/Dial/Hangup
   layouts, transaction ids, SMS route) instead of only changing the default
   version.
4. Verify in layers: IMS PDN + `Registered` → MO/MT call → two-way audio →
   SMS with acknowledgements → reboot/standby stability.

Do not copy Motorola-specific DSP partition fixes or other devices' radio blobs.
Turning IMS on also requires the system policy (`Registration=auto` / 4G Calling)
to allow registration; that is a runtime setting, not a driver substitute.

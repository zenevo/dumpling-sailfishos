# SSH / USB / Wi-Fi stability fixes

These fixes are part of the image since the **2026-11-11** build. They were
developed and verified on a real OnePlus 5T running this port; the image now
ships them by default.

They address three unrelated faults that all looked like "SSH is broken":

| Symptom | Root cause |
|---|---|
| `adb`/RNDIS works, but `ssh 192.168.2.15` times out over USB | the phone firewall defaults to `DROP` and never allowed TCP/22 on `usb0`; the DHCP request rule (68 → 67) was missing too, so the PC often got no lease at all |
| SSH refuses new connections every now and then | `sshd.socket` (socket activation, `Accept=yes`) and a standalone `sshd.service` were fighting over port 22; `Conflicts=` does not order the listener handover on systemd 238 |
| established SSH sessions drop "randomly" on Wi-Fi | Qualcomm LFR3 fast-roaming hand-off failures (`LFR3:Hand-Off Failed` → `Roam Synch Failed` → `WLAN_STA_DISCONNECT`), followed by a re-association |

A fourth issue was found while validating the fixes on reboot: ConnMan's IPv6
mangle chain rebuild failed (`Failed to flush table 'mangle'`), the rpfilter
rule was lost and the failsafe left `INPUT` at `ACCEPT`. That is a firewall
hole, so it is fixed here as well.

## What changed

| Path | Purpose |
|---|---|
| `/etc/connman/firewall.d/20-usb0-ssh-dhcp-firewall.conf` | allow TCP/22 and DHCP (UDP 68 → 67) on `usb0` only; managed by ConnMan's firewall lifecycle, so it survives restarts and re-plugs |
| `/etc/systemd/system/sshd.socket.d/10-ordered-activation.conf` | `After=sshd.service` — order the handover from the socket side |
| `/etc/systemd/system/sshd.service.d/10-ordered-standalone.conf` | `Conflicts=sshd.socket` + `Before=sshd.socket` — order it from the standalone side too |
| `/etc/systemd/system/sshd-watchdog.service`, `sshd-watchdog.timer` | recover an **enabled but inactive** `sshd.socket` (60 s after boot, then every 30 s) |
| `/usr/sbin/dumpling-sshd-watchdog` | the watchdog itself: no-op while the socket or a standalone server is running, idempotent `systemctl start`, real error reporting |
| `/etc/systemd/system/connman.service.d/20-dumpling-mangle-handover.conf` | run the helper around ConnMan start/stop |
| `/usr/sbin/dumpling-connman-mangle` | hand the known `connman-PREROUTING` rpfilter rule over to native `ip6tables` while ConnMan rebuilds its tables, then remove the temporary rule |
| `/usr/sbin/dumpling-wifi-roam` | narrow `GETROAMMODE` / `SETROAMMODE` ioctl tool (defaults to read-only) |
| `/etc/systemd/system/dumpling-wifi-stability.{service,timer}` | disable LFR on boot and re-apply it every ~30 s (also after a driver or supplicant reset) |
| `/etc/systemd/system/wpa_supplicant.service.d/20-dumpling-wifi-stability.conf` | pull the timer in whenever `wpa_supplicant` starts |
| `/etc/systemd/system/timers.target.wants/*` | enable both timers in the image |

## Behaviour and trade-offs

- **USB**: no more manual `iptables` rules and no dependency on the old
  `usb0-firewall.timer` (which on this image never fired at all). Plug the
  cable in and the PC gets `192.168.2.1/24`, then `ssh defaultuser@192.168.2.15`.
- **sshd**: both listener modes still work, but now in a defined order. If you
  switch modes the watchdog will not fight you: it only touches an *enabled*
  socket that is *not* active while no standalone server is running.
- **Wi-Fi**: LFR (Qualcomm fast roaming) is **disabled**. The phone no longer
  drops the link on a failed fast-roam hand-off, at the cost of slower/less
  seamless roaming between APs. Re-enable it with:

  ```sh
  sudo systemctl disable --now dumpling-wifi-stability.timer
  sudo /usr/sbin/dumpling-wifi-roam --restore-mode 0   # 0 = LFR on
  ```

- The ioctl ABI (struct layout, request `0x89F1`) is specific to the
  `qcacld` driver in this port's 4.4 kernel. The helper verifies every write by
  reading the value back and restores the previous value on mismatch.

## Verification performed on the device

- USB: DHCP lease `192.168.2.1/24`, direct route over `usb0`, SSH and
  `devel-su` both work; after a ConnMan restart the rules are rebuilt.
- IPv4/IPv6 `INPUT` policies stay `DROP`; the native rpfilter rule is back
  after ConnMan starts.
- socket → standalone → socket switching passes with `NRestarts=0`; both USB
  and Wi-Fi re-authenticate afterwards.
- 600 s Wi-Fi session with the screen off completed with SSH exit code 0 and no
  new LFR3 failures or `WLAN_STA_DISCONNECT` events in the kernel log.
- A 630 s journal capture over USB showed no new disconnects.

## Disabling / reverting

Each fix is independent:

```sh
sudo systemctl disable --now sshd-watchdog.timer          # watchdog only
sudo systemctl disable --now dumpling-wifi-stability.timer # Wi-Fi mitigation
sudo rm /etc/connman/firewall.d/20-usb0-ssh-dhcp-firewall.conf && sudo systemctl restart connman
```

The sshd ordering drop-ins are two-line unit overrides and are safe to leave in
place; remove the `.d/` directories if you want stock behaviour.

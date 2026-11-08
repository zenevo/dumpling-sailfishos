%package -n patterns-sailfish-device-configuration-dumpling
Summary: Jolla Configuration dumpling
Requires: patterns-sailfish-device-configuration-common-dumpling
Requires: patterns-sailfish-device-adaptation-dumpling

# For devices with a cellular modem. Those without one, comment out:
Requires: patterns-sailfish-cellular-apps
# Early stages of porting benefit from these:
Requires: patterns-sailfish-device-tools

Requires: sailfish-content-graphics-z%{icon_res}

# For multi-SIM devices
Requires: jolla-settings-networking-multisim

# 3rd party accounts like Twitter, VK, cloud services, etc
Requires: jolla-settings-accounts-extensions-3rd-party-all

# Required for Jolla Store Access
Requires: patterns-sailfish-consumer-generic

# For Mozilla location services (online)
Requires: geoclue-provider-mlsdb

# Sailfish OS CSD tool for hardware testing
Requires: csd

# Devices with 2G or more memory should also include this booster
# to improve camera startup times and the like
Requires: mapplauncherd-booster-silica-qt5-media

# OnePlus 5T hardware extras
Requires: jolla-settings-system-flashlight
Requires: jolla-settings-system-nfc
Requires: nfcd-dbuslog-plugin
Requires: nfcd-mce-plugin

# Fingerprint unlocking daemon for community ports (rear FPC sensor)
#Requires: sailfish-fpd-community
Requires: sailfish-devicelock-fpd
#Requires: droid-biometry-fp

# Enable /home encryption (needs an LVM/partition setup done at flash time)
#Requires: sailfish-device-encryption-community

# SELinux
#Requires: libselinux-utils
#Requires: libselinux-utils-extra
#Requires: setools
#Requires: policycoreutils

# Extras
Recommends: jolla-developer-mode
Recommends: strace
Recommends: vim-enhanced
Recommends: less
Recommends: busybox-static
Recommends: openssh-server
Recommends: zypper
Recommends: openssh-clients
Recommends: htop
Recommends: nano
Recommends: rsync

# I prefer proper bash thank you.
Recommends: gnu-bash
Obsoletes: busybox-symlinks-bash

%description -n patterns-sailfish-device-configuration-dumpling
Pattern with packages for dumpling configurations

%files -n patterns-sailfish-device-configuration-dumpling

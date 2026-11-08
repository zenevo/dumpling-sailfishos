# Reference: ../droid-configs-device/droid-configs.inc
#
# OnePlus 5T (dumpling) — Sailfish OS 5.2.0.22 (Finlayson)
# Community adaptation on LineageOS 16.0 / Android 9 (Pie) base.

%define vendor oneplus
%define device dumpling

%define vendor_pretty OnePlus
%define device_pretty OnePlus 5T (A5010)

%define community_adaptation 1
%define pixel_ratio 1.75

# Device-specific usb-moded configuration
Provides: usb-moded-configs
Obsoletes: usb-moded-defaults

# Device-specific ofono configuration
Provides: ofono-configs
Obsoletes: ofono-configs-mer

%define ofono_enable_plugins bluez5,hfp_ag_bluez5
%define ofono_disable_plugins bluez4,dun_gw_bluez4,hfp_ag_bluez4,hfp_bluez4,dun_gw_bluez5,hfp_bluez5

%include droid-configs-device/droid-configs.inc
%include patterns/patterns-sailfish-device-adaptation-dumpling.inc
%include patterns/patterns-sailfish-device-configuration-dumpling.inc

# Comparative Firmware Security Audit: SOHO (2012) vs. Enterprise (2025)

## Target 1: TP-Link TL-WR841N v7 (Consumer SOHO, 2012)
* **Firmware:** `wr841nv7_en_3_13_9_up(120201).bin` (Release Date: Feb 2012)
* **Architecture:** MIPS 32-bit Big-Endian (Atheros AR7241 / Linux 2.6.31)
* **Security Posture:** **Extremely Weak / Insecure by Design**

---

## Target 2: Huawei eKitEngine AP265E (Enterprise Access Point, 2025)
* **Firmware:** `AP162E-AP265E-AP361E-AP362E_V200R025C00SPC100.cc` (Release Date: Jan 2025)
* **Architecture:** ARM 32-bit Little-Endian (HiSilicon Hi1152 / Linux 5.10 openEuler)
* **Security Posture:** **Hardened / Defense-in-Depth**

---

## 📊 Comprehensive Comparison Matrix

| Security Feature / Dimension | TP-Link TL-WR841N v7 (2012) | Huawei eKitEngine AP265E (2025) | Security Assessment |
| :--- | :--- | :--- | :--- |
| **Factory Backdoors** | 🔴 **CRITICAL**: Hardcoded root backdoor (`/userRpmNatDebugRpm26525557/linux_cmdline.html` with `osteam:5up`) | 🟢 **NONE**: Zero undocumented web debug backdoors in production image | TP-Link left factory test scripts live in shipping retail units. |
| **Remote Kernel Module Injection** | 🔴 **CRITICAL**: Unauthenticated TFTP kernel injection (`insmod /tmp/art.ko`) | 🟢 **SECURED**: Kernel module signature enforcement; secure boot verification | TP-Link allows arbitrary kernel code execution over HTTP. |
| **Default Credentials** | 🔴 **WEAK**: Default credentials (`admin:admin`) with no forced password rotation | 🟢 **HARDENED**: KMC AES-GCM cipher (`$1b$`) with mandatory change on first login (`first_login=y`) | Huawei enforces complex password generation before granting access. |
| **Access Control (Routing)** | 🔴 **BROKEN**: Flat routing; static `.htm` files queryable directly | 🟢 **SECURED**: Virtual memory mapping from `web.7z`; strict `token.ini` validation (returns 404 for unauthorized probes) | Huawei prevents URL enumeration and unauthorized page viewing. |
| **Binary Hardening** | 🔴 **NONE**: No Stack Canaries, No ASLR, Executable Stack (`rwx`) | 🟢 **MODERN**: Stack Canaries (`-fstack-protector-strong`), ASLR, NX / DEP, PIE | Any memory corruption in TP-Link results in immediate reliable shellcode execution. |
| **Network Architecture** | 🔴 **VULNERABLE**: SOHO flat network; all LAN ports and Wi-Fi bridged | 🟢 **ISOLATED**: FIT AP architecture; control plane separated via CAPWAP DTLS; Layer 2 & Layer 3 User Isolation | Huawei isolates clients at the radio and MAC level (`USER_ISOLATE=1`). |

---

## 🔍 Deep Dive: How the Two Worlds Differ

### 1. Backdoor Handling
* **TP-Link 2012:** To test hardware in the factory, developers created `CmdRpmHtm` and `ArtRpmHtm`. Rather than stripping these functions with `#ifdef DEBUG`, they shipped them to millions of consumer homes. Anyone who knows the secret URL (`/userRpmNatDebugRpm26525557/...`) has full root access.
* **Huawei 2025:** Engineering commands (`_vrphidecmd`, `diagnose`) require physical console connection and high-privilege AAA authentication. Web endpoints are compiled directly into the monolithic `vos.o` daemon with strict token verification.

### 2. Password Architecture
* **TP-Link 2012:** Plaintext base64 Basic Authentication header parsed by `httpPwdDecode()`.
* **Huawei 2025:** Passwords are never stored in plaintext or reversible ciphers. System uses modern KMC Key Management Center with salt, preventing rainbow table attacks and firmware extraction compromises.

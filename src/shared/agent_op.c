/* Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"

/* Check if syscheck is to be executed/restarted
 * Returns 1 on success or 0 on failure (shouldn't be executed now)
 */
int os_check_restart_syscheck()
{
    /* If the restart is not present, return 0 */
    if (isChroot()) {
        if (unlink(SYSCHECK_RESTART) == -1) {
            return (0);
        }
    } else {
        if (unlink(SYSCHECK_RESTART_PATH) == -1) {
            return (0);
        }
    }

    return (1);
}

/* Set syscheck to be restarted
 * Returns 1 on success or 0 on failure
 */
int os_set_restart_syscheck()
{
    FILE *fp;

    fp = fopen(SYSCHECK_RESTART, "w");
    if (!fp) {
        merror(FOPEN_ERROR, SYSCHECK_RESTART, errno, strerror(errno));
        return (0);
    }

    fprintf(fp, "%s\n", SYSCHECK_RESTART);
    fclose(fp);

    return (1);
}

/* Read the agent name for the current agent
 * Returns NULL on error
 */
char *os_read_agent_name()
{
    char buf[1024 + 1];
    FILE *fp = NULL;

    mdebug2("Calling os_read_agent_name().");

    if (isChroot()) {
        fp = fopen(AGENT_INFO_FILE, "r");
    } else {
        fp = fopen(AGENT_INFO_FILEP, "r");
    }

    /* We give 1 second for the file to be created */
    if (!fp) {
        sleep(1);

        if (isChroot()) {
            fp = fopen(AGENT_INFO_FILE, "r");
        } else {
            fp = fopen(AGENT_INFO_FILEP, "r");
        }
    }

    if (!fp) {
        mdebug1(FOPEN_ERROR, AGENT_INFO_FILE, errno, strerror(errno));
        return (NULL);
    }

    buf[1024] = '\0';

    /* Get name */
    if (fgets(buf, 1024, fp)) {
        char *ret = NULL;
        int len;

        // strip the newlines
        len = strlen(buf) - 1;
        while (len > 0 && buf[len] == '\n')
            buf[len--] = '\0';

        os_strdup(buf, ret);
        fclose(fp);

        mdebug2("os_read_agent_name returned (%s).", ret);

        return (ret);
    }

    fclose(fp);
    return (NULL);
}

/* Read the agent ip for the current agent
 * Returns NULL on error
 */
char *os_read_agent_ip()
{
    char buf[1024 + 1];
    FILE *fp;

    mdebug2("Calling os_read_agent_ip().");

    fp = fopen(AGENT_INFO_FILE, "r");
    if (!fp) {
        merror(FOPEN_ERROR, AGENT_INFO_FILE, errno, strerror(errno));
        return (NULL);
    }

    buf[1024] = '\0';

    /* Get IP */
    if (fgets(buf, 1024, fp) && fgets(buf, 1024, fp)) {
        char *ret = NULL;
        os_strdup(buf, ret);
        fclose(fp);

        return (ret);
    }

    fclose(fp);
    return (NULL);
}

/* Read the agent id for the current agent
 * Returns NULL on error
 */
char *os_read_agent_id()
{
    char buf[1024 + 1];
    FILE *fp;

    mdebug2("Calling os_read_agent_id().");

    fp = fopen(AGENT_INFO_FILE, "r");
    if (!fp) {
        merror(FOPEN_ERROR, AGENT_INFO_FILE, errno, strerror(errno));
        return (NULL);
    }

    buf[1024] = '\0';

    /* Get id */
    if (fgets(buf, 1024, fp) && fgets(buf, 1024, fp) && fgets(buf, 1024, fp)) {
        char *ret = NULL;
        os_strdup(buf, ret);
        fclose(fp);

        return (ret);
    }

    fclose(fp);
    return (NULL);
}

/*  Read the agent profile name for the current agent
 *  Returns NULL on error
 *
 *  Description:
 *  Comma separated list of strings that used to identify what type
 *  of configuration is used for this agent.
 *  The profile name is set in the agent's etc/ossec.conf file
 *  It is matched with the ossec manager's agent.conf file to read
 *  configuration only applicable to this profile name.
 */
char *os_read_agent_profile()
{
    char buf[1024 + 1];
    FILE *fp;

    mdebug2("Calling os_read_agent_profile().");

    if (isChroot()) {
        fp = fopen(AGENT_INFO_FILE, "r");
    } else {
        fp = fopen(AGENT_INFO_FILEP, "r");
    }

    if (!fp) {
        mdebug2("Failed to open file. Errno=%d.", errno);
        merror(FOPEN_ERROR, AGENT_INFO_FILE, errno, strerror(errno));
        return (NULL);
    }

    buf[1024] = '\0';

    /* Get profile */
    if (fgets(buf, 1024, fp) && fgets(buf, 1024, fp) &&
            fgets(buf, 1024, fp) && fgets(buf, 1024, fp)) {
        char *ret = NULL;

        /* Trim the /n and/or /r at the end of the string */
        os_trimcrlf(buf);

        os_strdup(buf, ret);
        mdebug2("os_read_agent_profile() = [%s]", ret);

        fclose(fp);

        return (ret);
    }

    fclose(fp);
    return (NULL);
}

/* Write the agent info to the queue, for the other processes to read
 * Returns 1 on success or <= 0 on failure
 */
int os_write_agent_info(const char *agent_name, __attribute__((unused)) const char *agent_ip,
                        const char *agent_id, const char *cfg_profile_name)
{
    FILE *fp;

    fp = fopen(AGENT_INFO_FILE, "w");
    if (!fp) {
        merror(FOPEN_ERROR, AGENT_INFO_FILE, errno, strerror(errno));
        return (0);
    }

    fprintf(
        fp,
        "%s\n-\n%s\n%s\n",
        agent_name,
        agent_id,
        (cfg_profile_name) ? cfg_profile_name : "-"
    );
    fclose(fp);
    return (1);
}

/* Read group. Returns 0 on success or -1 on failure. */
int get_agent_group(const char *id, char *group, size_t size) {
    char path[PATH_MAX];
    int result = 0;
    FILE *fp;

    if (snprintf(path, PATH_MAX, isChroot() ? GROUPS_DIR "/%s" : DEFAULTDIR GROUPS_DIR "/%s", id) >= PATH_MAX) {
        merror("At get_agent_group(): file path too large for agent '%s'.", id);
        return -1;
    }

    if (!(fp = fopen(path, "r"))) {
        mdebug1("At get_agent_group(): file '%s' not found.", path);
        return -1;
    }

    if (fgets(group, size, fp)) {
        char *endl = strchr(group, '\n');

        if (endl) {
            *endl = '\0';
        }
    } else {
        mwarn("Empty group for agent ID '%s'.", id);
        result = -1;
    }

    fclose(fp);
    return result;
}

/* Set agent group. Returns 0 on success or -1 on failure. */
int set_agent_group(const char * id, const char * group) {
    char path[PATH_MAX];
    FILE *fp;
    mode_t oldmask;

    if (snprintf(path, PATH_MAX, isChroot() ? GROUPS_DIR "/%s" : DEFAULTDIR GROUPS_DIR "/%s", id) >= PATH_MAX) {
        merror("At set_agent_group(): file path too large for agent '%s'.", id);
        return -1;
    }

    oldmask = umask(0006);
    fp = fopen(path, "w");
    umask(oldmask);

    if (!fp) {
        mdebug1("At get_agent_group(): file '%s' not found.", path);
        return -1;
    }

    fprintf(fp, "%s\n", group);
    fclose(fp);
    return 0;
}

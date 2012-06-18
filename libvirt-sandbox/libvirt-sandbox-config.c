/*
 * libvirt-sandbox-config.c: libvirt sandbox configuration
 *
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>
#include <sys/utsname.h>
#include <string.h>

#include "libvirt-sandbox/libvirt-sandbox.h"

/**
 * SECTION: libvirt-sandbox-config
 * @short_description: Basic sandbox configuration details
 * @include: libvirt-sandbox/libvirt-sandbox.h
 * @see_also: #GVirSandboxConfigGraphical
 *
 * Provides a base object to store configurations for the application sandbox
 *
 * The GVirSandboxConfig object stores the basic information required to
 * create application sandboxes with a simple text based console.
 */

#define GVIR_SANDBOX_CONFIG_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_SANDBOX_TYPE_CONFIG, GVirSandboxConfigPrivate))

struct _GVirSandboxConfigPrivate
{
    gchar *name;
    gchar *root;
    gchar *arch;
    gchar *kernrelease;
    gchar *kernpath;
    gboolean shell;

    guint uid;
    guint gid;
    gchar *username;
    gchar *homedir;

    gulong memory;

    GList *networks;
    GList *hostBindMounts;
    GList *guestBindMounts;
    GList *hostImageMounts;

    gchar *secLabel;
    gboolean secDynamic;
};

G_DEFINE_ABSTRACT_TYPE(GVirSandboxConfig, gvir_sandbox_config, G_TYPE_OBJECT);


enum {
    PROP_0,

    PROP_NAME,
    PROP_ROOT,
    PROP_ARCH,
    PROP_SHELL,
    PROP_KERNRELEASE,
    PROP_KERNPATH,

    PROP_UID,
    PROP_GID,
    PROP_USERNAME,
    PROP_HOMEDIR,

    PROP_MEMORY,

    PROP_SECURITY_LABEL,
    PROP_SECURITY_DYNAMIC,
};

enum {
    LAST_SIGNAL
};

//static gint signals[LAST_SIGNAL];

static gboolean gvir_sandbox_config_load_config(GVirSandboxConfig *config,
                                                GKeyFile *file,
                                                GError **error);
static void gvir_sandbox_config_save_config(GVirSandboxConfig *config,
                                            GKeyFile *file);

#define GVIR_SANDBOX_CONFIG_ERROR gvir_sandbox_config_error_quark()

static GQuark
gvir_sandbox_config_error_quark(void)
{
    return g_quark_from_static_string("gvir-sandbox-config");
}

static void gvir_sandbox_config_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxConfig *config = GVIR_SANDBOX_CONFIG(object);
    GVirSandboxConfigPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;

    case PROP_ROOT:
        g_value_set_string(value, priv->root);
        break;

    case PROP_ARCH:
        g_value_set_string(value, priv->arch);
        break;

    case PROP_KERNRELEASE:
        g_value_set_string(value, priv->kernrelease);
        break;

    case PROP_KERNPATH:
        g_value_set_string(value, priv->kernpath);
        break;

    case PROP_SHELL:
        g_value_set_boolean(value, priv->shell);
        break;

    case PROP_UID:
        g_value_set_uint(value, priv->uid);
        break;

    case PROP_GID:
        g_value_set_uint(value, priv->gid);
        break;

    case PROP_USERNAME:
        g_value_set_string(value, priv->username);
        break;

    case PROP_HOMEDIR:
        g_value_set_string(value, priv->homedir);
        break;

    case PROP_MEMORY:
        g_value_set_ulong(value, priv->memory);
        break;

    case PROP_SECURITY_LABEL:
        g_value_set_string(value, priv->secLabel);
        break;

    case PROP_SECURITY_DYNAMIC:
        g_value_set_boolean(value, priv->secDynamic);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    GVirSandboxConfig *config = GVIR_SANDBOX_CONFIG(object);
    GVirSandboxConfigPrivate *priv = config->priv;

    switch (prop_id) {
    case PROP_NAME:
        g_free(priv->name);
        priv->name = g_value_dup_string(value);
        break;

    case PROP_ROOT:
        g_free(priv->root);
        priv->root = g_value_dup_string(value);
        break;

    case PROP_ARCH:
        g_free(priv->arch);
        priv->arch = g_value_dup_string(value);
        break;

    case PROP_KERNRELEASE:
        g_free(priv->kernrelease);
        priv->kernrelease = g_value_dup_string(value);
        break;

    case PROP_KERNPATH:
        g_free(priv->kernpath);
        priv->kernpath = g_value_dup_string(value);
        break;

    case PROP_SHELL:
        priv->shell = g_value_get_boolean(value);
        break;

    case PROP_UID:
        priv->uid = g_value_get_uint(value);
        break;

    case PROP_GID:
        priv->gid = g_value_get_uint(value);
        break;

    case PROP_USERNAME:
        g_free(priv->username);
        priv->username = g_value_dup_string(value);
        break;

    case PROP_HOMEDIR:
        g_free(priv->homedir);
        priv->homedir = g_value_dup_string(value);
        break;

    case PROP_MEMORY:
        priv->memory = g_value_get_ulong(value);
        break;

    case PROP_SECURITY_LABEL:
        g_free(priv->secLabel);
        priv->secLabel = g_value_dup_string(value);
        break;

    case PROP_SECURITY_DYNAMIC:
        priv->secDynamic = g_value_get_boolean(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_sandbox_config_finalize(GObject *object)
{
    GVirSandboxConfig *config = GVIR_SANDBOX_CONFIG(object);
    GVirSandboxConfigPrivate *priv = config->priv;

    g_list_foreach(priv->hostBindMounts, (GFunc)g_object_unref, NULL);
    g_list_free(priv->hostBindMounts);

    g_list_foreach(priv->guestBindMounts, (GFunc)g_object_unref, NULL);
    g_list_free(priv->guestBindMounts);

    g_list_foreach(priv->hostImageMounts, (GFunc)g_object_unref, NULL);
    g_list_free(priv->hostImageMounts);

    g_list_foreach(priv->networks, (GFunc)g_object_unref, NULL);
    g_list_free(priv->networks);


    g_free(priv->name);
    g_free(priv->root);
    g_free(priv->arch);
    g_free(priv->kernrelease);
    g_free(priv->kernpath);
    g_free(priv->secLabel);

    G_OBJECT_CLASS(gvir_sandbox_config_parent_class)->finalize(object);
}


static void gvir_sandbox_config_class_init(GVirSandboxConfigClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gvir_sandbox_config_finalize;
    object_class->get_property = gvir_sandbox_config_get_property;
    object_class->set_property = gvir_sandbox_config_set_property;

    klass->load_config = gvir_sandbox_config_load_config;
    klass->save_config = gvir_sandbox_config_save_config;

    g_object_class_install_property(object_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Name",
                                                        "The sandbox name",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_ROOT,
                                    g_param_spec_string("root",
                                                        "Root",
                                                        "The sandbox root",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_ARCH,
                                    g_param_spec_string("arch",
                                                        "Arch",
                                                        "The sandbox architecture",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_KERNRELEASE,
                                    g_param_spec_string("kernrelease",
                                                        "Kernrelease",
                                                        "The kernel release version",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_KERNPATH,
                                    g_param_spec_string("kernpath",
                                                        "Kernpath",
                                                        "The kernel image path",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SHELL,
                                    g_param_spec_string("shell",
                                                        "SHELL",
                                                        "SHELL",
                                                        FALSE,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_UID,
                                    g_param_spec_uint("uid",
                                                      "UID",
                                                      "The user ID",
                                                      0,
                                                      G_MAXUINT,
                                                      geteuid(),
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_GID,
                                    g_param_spec_uint("gid",
                                                      "GID",
                                                      "The group ID",
                                                      0,
                                                      G_MAXUINT,
                                                      getegid(),
                                                      G_PARAM_READABLE |
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_USERNAME,
                                    g_param_spec_string("username",
                                                        "Username",
                                                        "The username",
                                                        g_get_user_name(),
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_HOMEDIR,
                                    g_param_spec_string("homedir",
                                                        "Homedir",
                                                        "The home directory",
                                                        g_get_home_dir(),
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_MEMORY,
                                    g_param_spec_ulong("memory",
                                                        "Memory",
                                                        "Max amount of memory used",
                                                        0,
							G_MAXULONG,
							0,		/* unlimited */
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SECURITY_LABEL,
                                    g_param_spec_string("security-label",
                                                        "Security label",
                                                        "The security label",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SECURITY_DYNAMIC,
                                    g_param_spec_boolean("security-dynamic",
                                                         "Security dynamic",
                                                         "The security mode",
                                                         TRUE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirSandboxConfigPrivate));
}


static void gvir_sandbox_config_init(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    struct utsname uts;

    uname(&uts);

    priv = config->priv = GVIR_SANDBOX_CONFIG_GET_PRIVATE(config);

    priv->name = g_strdup("sandbox");
    priv->root = g_strdup("/");
    priv->arch = g_strdup(uts.machine);
    priv->kernrelease = g_strdup(uts.release);
    priv->kernpath = g_strdup_printf("/boot/vmlinuz-%s", priv->kernrelease);
    priv->secLabel = g_strdup("system_u:system_r:svirt_t:s0:c0.c1023");

    priv->uid = geteuid();
    priv->gid = getegid();
    priv->username = g_strdup(g_get_user_name());
    priv->homedir = g_strdup(g_get_home_dir());
}


/**
 * gvir_sandbox_config_get_name:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox name
 *
 * Returns: (transfer none): the current name
 */
const gchar *gvir_sandbox_config_get_name(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->name;
}


/**
 * gvir_sandbox_config_set_root:
 * @config: (transfer none): the sandbox config
 * @hostdir: (transfer none): the host directory
 *
 * Set the host directory to use as the root for the sandbox. The
 * defualt root is "/".
 */
void gvir_sandbox_config_set_root(GVirSandboxConfig *config, const gchar *hostdir)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->root);
    priv->root = g_strdup(hostdir);
}

/**
 * gvir_sandbox_config_get_root:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox root directory
 *
 * Returns: (transfer none): the current root
 */
const gchar *gvir_sandbox_config_get_root(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->root;
}


/**
 * gvir_sandbox_config_set_arch:
 * @config: (transfer none): the sandbox config
 * @arch: (transfer none): the host directory
 *
 * Set the architecture to use in the sandbox. If none is provided,
 * it will default to matching the host architecture.
 */
void gvir_sandbox_config_set_arch(GVirSandboxConfig *config, const gchar *arch)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->arch);
    priv->arch = g_strdup(arch);
}


/**
 * gvir_sandbox_config_get_arch:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox binary architecture
 *
 * Returns: (transfer none): the current architecture
 */
const gchar *gvir_sandbox_config_get_arch(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->arch;
}


/**
 * gvir_sandbox_config_set_kernrelease:
 * @config: (transfer none): the sandbox config
 * @kernrelease: (transfer none): the host directory
 *
 * Set the kernel release version to use in the sandbox. If none is provided,
 * it will default to matching the current running kernel.
 * Also sets the default kernel path as /boot/vmlinuz-<release>
 */
void gvir_sandbox_config_set_kernrelease(GVirSandboxConfig *config, const gchar *kernrelease)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->kernrelease);
    priv->kernrelease = g_strdup(kernrelease);
    gvir_sandbox_config_set_kernpath(config, g_strdup_printf("/boot/vmlinuz-%s", priv->kernrelease));

}


/**
 * gvir_sandbox_config_get_kernrelease:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox kernel release version
 *
 * Returns: (transfer none): the current kernel release version
 */
const gchar *gvir_sandbox_config_get_kernrelease(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->kernrelease;
}
/**
 * gvir_sandbox_config_set_kernpath:
 * @config: (transfer none): the sandbox config
 * @kernpath: (transfer none): the host directory
 *
 * Set the kernel image path to use in the sandbox. If none is provided,
 * it will default to matching /boot/vmlinuz-<kernel release>.
 */

void gvir_sandbox_config_set_kernpath(GVirSandboxConfig *config, const gchar *kernpath)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->kernpath);
    priv->kernpath = g_strdup(kernpath);
}


/**
 * gvir_sandbox_config_get_kernpath:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox kernel image path
 *
 * Returns: (transfer none): the current kernel image path
 */
const gchar *gvir_sandbox_config_get_kernpath(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->kernpath;
}


/**
 * gvir_sandbox_config_set_shell:
 * @config: (transfer none): the sandbox config
 * @shell: (transfer none): true if the container should have a shell
 *
 * Set whether the container console should have a interactive shell.
 */
void gvir_sandbox_config_set_shell(GVirSandboxConfig *config, gboolean shell)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->shell = shell;
}


/**
 * gvir_sandbox_config_get_shell:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox shell flag
 *
 * Returns: (transfer none): the shell flag
 */
gboolean gvir_sandbox_config_get_shell(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->shell;
}


/**
 * gvir_sandbox_config_set_userid:
 * @config: (transfer none): the sandbox config
 * @uid: (transfer none): the sandbox user ID
 *
 * Set the user ID to invoke the sandbox application under.
 * Defaults to the user ID of the current program.
 */
void gvir_sandbox_config_set_userid(GVirSandboxConfig *config, guint uid)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->uid = uid;
}


/**
 * gvir_sandbox_config_get_userid:
 * @config: (transfer none): the sandbox config
 *
 * Get the user ID to invoke the sandbox application under.
 *
 * Returns: (transfer none): the user ID
 */
guint gvir_sandbox_config_get_userid(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->uid;
}


/**
 * gvir_sandbox_config_set_groupid:
 * @config: (transfer none): the sandbox config
 * @gid: (transfer none): the sandbox group ID
 *
 * Set the group ID to invoke the sandbox application under.
 * Defaults to the group ID of the current program.
 */
void gvir_sandbox_config_set_groupid(GVirSandboxConfig *config, guint gid)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->gid = gid;
}


/**
 * gvir_sandbox_config_get_groupid:
 * @config: (transfer none): the sandbox config
 *
 * Get the group ID to invoke the sandbox application under.
 *
 * Returns: (transfer none): the group ID
 */
guint gvir_sandbox_config_get_groupid(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->gid;
}


/**
 * gvir_sandbox_config_set_username:
 * @config: (transfer none): the sandbox config
 * @username: (transfer none): the sandbox user name
 *
 * Set the user name associated with the sandbox user ID.
 * Defaults to the user name of the current program.
 */
void gvir_sandbox_config_set_username(GVirSandboxConfig *config, const gchar *username)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->username);
    priv->username = g_strdup(username);
}


/**
 * gvir_sandbox_config_get_username:
 * @config: (transfer none): the sandbox config
 *
 * Get the user name to invoke the sandbox application under.
 *
 * Returns: (transfer none): the user name
 */
const gchar *gvir_sandbox_config_get_username(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->username;
}


/**
 * gvir_sandbox_config_set_homedir:
 * @config: (transfer none): the sandbox config
 * @homedir: (transfer none): the sandbox home directory
 *
 * Set the home directory associated with the sandbox user ID.
 * Defaults to the home directory of the current program.
 */
void gvir_sandbox_config_set_homedir(GVirSandboxConfig *config, const gchar *homedir)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->homedir);
    priv->homedir = g_strdup(homedir);
}


/**
 * gvir_sandbox_config_get_homedir:
 * @config: (transfer none): the sandbox config
 *
 * Get the home directory associated with the sandbox user ID
 *
 * Returns: (transfer none): the home directory
 */
const gchar *gvir_sandbox_config_get_homedir(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->homedir;
}



/**
 * gvir_sandbox_config_add_network:
 * @config: (transfer none): the sandbox config
 * @network: (transfer none): the network configuration
 *
 * Adds a new network connection to the sandbox
 */
void gvir_sandbox_config_add_network(GVirSandboxConfig *config,
                                     GVirSandboxConfigNetwork *network)
{
    GVirSandboxConfigPrivate *priv = config->priv;

    g_object_ref(network);
    priv->networks = g_list_append(priv->networks, network);
}


/**
 * gvir_sandbox_config_get_networks:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the list of custom networks in the sandbox
 *
 * Returns: (transfer full) (element-type GVirSandboxConfigNetwork): the list of networks
 */
GList *gvir_sandbox_config_get_networks(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_list_foreach(priv->networks, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->networks);
}


/**
 * gvir_sandbox_config_add_network_strv:
 * @config: (transfer none): the sandbox config
 * @networks: (transfer none)(array zero-terminated=1): the list of networks
 *
 * Parses @networks whose elements are in the format
 * KEY=VALUE, creating #GVirSandboxConfigNetwork
 * instances for each element.
 *
 *  dhcp
 *  address=192.168.122.1/24%192.168.122.255;
 *  address=192.168.122.1/24%192.168.122.255;address=2001:212::204.2/64
 *  route=192.168.122.255/24%192.168.1.1
 */
gboolean gvir_sandbox_config_add_network_strv(GVirSandboxConfig *config,
                                              gchar **networks,
                                              GError **error)
{
    gboolean ret = FALSE;
    gsize i = 0;
    while (networks && networks[i]) {
        gchar **params = g_strsplit(networks[i], ";", 50);
        gsize j = 0;
        GVirSandboxConfigNetwork *net;

        net = gvir_sandbox_config_network_new();

        while (params && params[j]) {
            gchar *param = params[j];

            if (g_str_equal(param, "dhcp")) {
                gvir_sandbox_config_network_set_dhcp(net, TRUE);
            } else if (g_str_has_prefix(param, "address=")) {
                GVirSandboxConfigNetworkAddress *addr;
                GInetAddress *primaryaddr;
                GInetAddress *bcastaddr;
                gchar *primary = g_strdup(param + strlen("address="));
                gchar *bcast = NULL;
                guint prefix = 24;
                gchar *tmp;
                if ((tmp = strchr(primary, '/'))) {
                    prefix = g_ascii_strtoll(tmp+1, NULL, 10);
                    *tmp = '\0';
                    tmp = strchr(tmp+1, '%');
                } else {
                    tmp = strchr(primary, '%');
                }
                if (tmp) {
                    *tmp = '\0';
                    bcast = tmp+1;
                }

                if (!(primaryaddr = g_inet_address_new_from_string(primary))) {
                    g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                                "Unable to parse address %s", primary);
                    g_free(primary);
                    goto cleanup;
                }

                if (!(bcastaddr = g_inet_address_new_from_string(bcast))) {
                    g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                                "Unable to parse address %s", bcast);
                    g_free(primary);
                    g_object_unref(primaryaddr);
                    goto cleanup;
                }

                addr = gvir_sandbox_config_network_address_new(primaryaddr,
                                                               prefix,
                                                               bcastaddr);

                gvir_sandbox_config_network_add_address(net, addr);

                g_object_unref(primaryaddr);
                g_object_unref(bcastaddr);
                g_free(primary);

                gvir_sandbox_config_network_set_dhcp(net, FALSE);
            } else if (g_str_has_prefix(param, "route=")) {
                GVirSandboxConfigNetworkRoute *route;
                GInetAddress *targetaddr;
                GInetAddress *gatewayaddr;
                gchar *target = g_strdup(param + strlen("route="));
                gchar *gateway = NULL;
                guint prefix = 24;
                gchar *tmp;
                if ((tmp = strchr(target, '/'))) {
                    prefix = g_ascii_strtoll(tmp+1, NULL, 10);
                    *tmp = '\0';
                    tmp = strchr(tmp+1, '%');
                } else {
                    tmp = strchr(target, '%');
                }
                if (tmp) {
                    *tmp = '\0';
                    gateway = tmp+1;
                }

                if (!(targetaddr = g_inet_address_new_from_string(target))) {
                    g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                                "Unable to parse address %s", target);
                    g_free(target);
                    goto cleanup;
                }

                if (!(gatewayaddr = g_inet_address_new_from_string(gateway))) {
                    g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                                "Unable to parse address %s", gateway);
                    g_free(target);
                    g_object_unref(targetaddr);
                    goto cleanup;
                }

                route = gvir_sandbox_config_network_route_new(targetaddr,
                                                              prefix,
                                                              gatewayaddr);

                gvir_sandbox_config_network_add_route(net, route);

                g_object_unref(targetaddr);
                g_object_unref(gatewayaddr);
                g_free(target);

                gvir_sandbox_config_network_set_dhcp(net, FALSE);
            }

            j++;
        }
        g_strfreev(params);

        gvir_sandbox_config_add_network(config, net);
        g_object_unref(net);
        i++;
    }
    ret = TRUE;
cleanup:
    return ret;
}


gboolean gvir_sandbox_config_has_networks(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->networks ? TRUE : FALSE;
}


/**
 * gvir_sandbox_config_add_host_bind_mount:
 * @config: (transfer none): the sandbox config
 * @mnt: (transfer none): the mount configuration
 *
 * Adds a new custom mount to the sandbox, to override part of the
 * host filesystem
 */
void gvir_sandbox_config_add_host_bind_mount(GVirSandboxConfig *config,
                                             GVirSandboxConfigMount *mnt)
{
    GVirSandboxConfigPrivate *priv = config->priv;

    g_object_ref(mnt);
    priv->hostBindMounts = g_list_append(priv->hostBindMounts, mnt);
}


/**
 * gvir_sandbox_config_get_host_bind_mounts:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the list of custom mounts in the sandbox
 *
 * Returns: (transfer full) (element-type GVirSandboxConfigMount): the list of mounts
 */
GList *gvir_sandbox_config_get_host_bind_mounts(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_list_foreach(priv->hostBindMounts, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->hostBindMounts);
}


/**
 * gvir_sandbox_config_find_host_bind_mount:
 * @config: (transfer none): the sandbox config
 * @target: the guest target path
 *
 * Finds the #GVirSandboxConfigMount object corresponding to a guest
 * path of @target.
 *
 * Returns: (transfer full)(allow-none): a mount object or NULL
 */
GVirSandboxConfigMount *gvir_sandbox_config_find_host_bind_mount(GVirSandboxConfig *config,
                                                                 const gchar *target)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    GList *tmp = priv->hostBindMounts;

    while (tmp) {
        GVirSandboxConfigMount *mnt = GVIR_SANDBOX_CONFIG_MOUNT(tmp->data);
        if (g_str_equal(gvir_sandbox_config_mount_get_target(mnt), target)) {
            return g_object_ref(mnt);
        }
        tmp = tmp->next;
    }
    return NULL;
}


/**
 * gvir_sandbox_config_add_host_bind_mount_strv:
 * @config: (transfer none): the sandbox config
 * @mounts: (transfer none)(array zero-terminated=1): the list of mounts
 *
 * Parses @mounts whose elements are in the format
 * GUEST-PATH=HOST-PATH, creating #GVirSandboxConfigMount
 * instances for each element.
 */
gboolean gvir_sandbox_config_add_host_bind_mount_strv(GVirSandboxConfig *config,
                                                      gchar **mounts,
                                                      GError **error G_GNUC_UNUSED)
{
    gsize i = 0;
    while (mounts && mounts[i]) {
        gchar *guest = NULL;
        const gchar *host = NULL;
        GVirSandboxConfigMount *mnt;
        gchar *tmp;

        guest = g_strdup(mounts[i]);

        if ((tmp = strchr(guest, '=')) != NULL) {
            *tmp = '\0';
            host = tmp + 1;
        }

        mnt = gvir_sandbox_config_mount_new(guest);
        gvir_sandbox_config_mount_set_root(mnt, host);

        gvir_sandbox_config_add_host_bind_mount(config, mnt);
        g_object_unref(mnt);
        g_free(guest);
        i++;
    }

    return TRUE;
}


/**
 * gvir_sandbox_config_add_host_image_mount:
 * @config: (transfer none): the sandbox config
 * @mnt: (transfer none): the mount configuration
 *
 * Adds a new custom mount to the sandbox, to override part of the
 * host filesystem
 */
void gvir_sandbox_config_add_host_image_mount(GVirSandboxConfig *config,
                                             GVirSandboxConfigMount *mnt)
{
    GVirSandboxConfigPrivate *priv = config->priv;

    g_object_ref(mnt);
    priv->hostImageMounts = g_list_append(priv->hostImageMounts, mnt);
}


/**
 * gvir_sandbox_config_get_host_image_mounts:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the list of custom mounts in the sandbox
 *
 * Returns: (transfer full) (element-type GVirSandboxConfigMount): the list of mounts
 */
GList *gvir_sandbox_config_get_host_image_mounts(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_list_foreach(priv->hostImageMounts, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->hostImageMounts);
}


/**
 * gvir_sandbox_config_find_host_image_mount:
 * @config: (transfer none): the sandbox config
 * @target: the guest target path
 *
 * Finds the #GVirSandboxConfigMount object corresponding to a guest
 * path of @target.
 *
 * Returns: (transfer full)(allow-none): a mount object or NULL
 */
GVirSandboxConfigMount *gvir_sandbox_config_find_host_image_mount(GVirSandboxConfig *config,
                                                                 const gchar *target)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    GList *tmp = priv->hostImageMounts;

    while (tmp) {
        GVirSandboxConfigMount *mnt = GVIR_SANDBOX_CONFIG_MOUNT(tmp->data);
        if (g_str_equal(gvir_sandbox_config_mount_get_target(mnt), target)) {
            return g_object_ref(mnt);
        }
        tmp = tmp->next;
    }
    return NULL;
}


/**
 * gvir_sandbox_config_add_host_image_mount_strv:
 * @config: (transfer none): the sandbox config
 * @mounts: (transfer none)(array zero-terminated=1): the list of mounts
 *
 * Parses @mounts whose elements are in the format
 * GUEST-PATH=HOST-PATH, creating #GVirSandboxConfigMount
 * instances for each element.
 */
gboolean gvir_sandbox_config_add_host_image_mount_strv(GVirSandboxConfig *config,
                                                      gchar **mounts,
                                                      GError **error G_GNUC_UNUSED)
{
    gsize i = 0;
    while (mounts && mounts[i]) {
        gchar *guest = NULL;
        const gchar *host = NULL;
        GVirSandboxConfigMount *mnt;
        gchar *tmp;

        guest = g_strdup(mounts[i]);

        if ((tmp = strchr(guest, '=')) != NULL) {
            *tmp = '\0';
            host = tmp + 1;
        }

        mnt = gvir_sandbox_config_mount_new(guest);
        gvir_sandbox_config_mount_set_root(mnt, host);

        gvir_sandbox_config_add_host_image_mount(config, mnt);
        g_object_unref(mnt);
        g_free(guest);
        i++;
    }

    return TRUE;
}


gboolean gvir_sandbox_config_has_host_image_mounts(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->hostImageMounts != NULL;
}


/**
 * gvir_sandbox_config_add_guest_bind_mount:
 * @config: (transfer none): the sandbox config
 * @mnt: (transfer none): the mount configuration
 *
 * Adds a new custom mount to the sandbox, to override part of the
 * bind filesystem
 */
void gvir_sandbox_config_add_guest_bind_mount(GVirSandboxConfig *config,
                                              GVirSandboxConfigMount *mnt)
{
    GVirSandboxConfigPrivate *priv = config->priv;

    g_object_ref(mnt);
    priv->guestBindMounts = g_list_append(priv->guestBindMounts, mnt);
}


/**
 * gvir_sandbox_config_get_guest_bind_mounts:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the list of custom mounts in the sandbox
 *
 * Returns: (transfer full) (element-type GVirSandboxConfigMount): the list of mounts
 */
GList *gvir_sandbox_config_get_guest_bind_mounts(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_list_foreach(priv->guestBindMounts, (GFunc)g_object_ref, NULL);
    return g_list_copy(priv->guestBindMounts);
}


/**
 * gvir_sandbox_config_find_guest_bind_mount:
 * @config: (transfer none): the sandbox config
 * @target: the guest target path
 *
 * Finds the #GVirSandboxConfigMount object corresponding to a guest
 * path of @target.
 *
 * Returns: (transfer full)(allow-none): a mount object or NULL
 */
GVirSandboxConfigMount *gvir_sandbox_config_find_guest_bind_mount(GVirSandboxConfig *config,
                                                                  const gchar *target)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    GList *tmp = priv->guestBindMounts;

    while (tmp) {
        GVirSandboxConfigMount *mnt = GVIR_SANDBOX_CONFIG_MOUNT(tmp->data);
        if (g_str_equal(gvir_sandbox_config_mount_get_target(mnt), target)) {
            return g_object_ref(mnt);
        }
        tmp = tmp->next;
    }
    return NULL;
}


/**
 * gvir_sandbox_config_add_guest_bind_mount_strv:
 * @config: (transfer none): the sandbox config
 * @mounts: (transfer none)(array zero-terminated=1): the list of mounts
 *
 * Parses @mounts whose elements are in the format
 * GUEST-PATH=BIND-PATH, creating #GVirSandboxConfigMount
 * instances for each element.
 */
gboolean gvir_sandbox_config_add_guest_bind_mount_strv(GVirSandboxConfig *config,
                                                       gchar **mounts,
                                                       GError **error G_GNUC_UNUSED)
{
    gsize i = 0;
    while (mounts && mounts[i]) {
        gchar *guest = NULL;
        const gchar *bind = NULL;
        GVirSandboxConfigMount *mnt;
        gchar *tmp;

        guest = g_strdup(mounts[i]);

        if ((tmp = strchr(guest, '=')) != NULL) {
            *tmp = '\0';
            bind = tmp + 1;
        }

        mnt = gvir_sandbox_config_mount_new(guest);
        gvir_sandbox_config_mount_set_root(mnt, bind);

        gvir_sandbox_config_add_guest_bind_mount(config, mnt);
        g_object_unref(mnt);
        g_free(guest);
        i++;
    }

    return TRUE;
}


/**
 * gvir_sandbox_config_add_host_include_strv:
 * @config: (transfer none): the sandbox config
 * @includes: (transfer none)(array zero-terminated=1): the list of includes
 *
 * Parses @includes whose elements are in the format
 * GUEST-TARGET=ROOT-PATH. If ROOT_PATH is omitted,
 * then it is assumed to be the same as GUEST-TARGET
 */
gboolean gvir_sandbox_config_add_host_include_strv(GVirSandboxConfig *config,
                                                   gchar **includes,
                                                   GError **error)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    gsize i = 0;

    while (includes && includes[i]) {
        const gchar *host;
        gchar *guest;
        const gchar *relguest;
        GVirSandboxConfigMount *mnt = NULL;
        GList *mnts = NULL;
        gchar *tmp;

        guest = g_strdup(includes[i]);
        if ((tmp = strchr(guest, '=')) != NULL) {
            *tmp = '\0';
            host = tmp + 1;
        } else {
            host = guest;
        }

        mnts = priv->hostBindMounts;
        while (mnts) {
            mnt = GVIR_SANDBOX_CONFIG_MOUNT(mnts->data);
            const gchar *target = gvir_sandbox_config_mount_get_target(mnt);
            if (g_str_has_prefix(guest, target)) {
                relguest = guest + strlen(target);
                break;
            }
            mnt = NULL;
            mnts = mnts->next;
        }
        if (!mnt) {
            g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                        "No mount with a prefix under %s", guest);
            g_free(guest);
            return FALSE;
        }

        gvir_sandbox_config_mount_add_include(mnt, host, relguest);
        g_free(guest);

        i++;
    }

    return TRUE;
}


gboolean gvir_sandbox_config_add_host_include_file(GVirSandboxConfig *config,
                                                   gchar *includefile,
                                                   GError **error)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    GFile *file = g_file_new_for_path(includefile);
    GFileInputStream *is = NULL;
    GDataInputStream *dis = NULL;
    gboolean ret = FALSE;
    gchar *line = NULL;

    if (!(is = g_file_read(file, NULL, error)))
        goto cleanup;

    dis = g_data_input_stream_new(G_INPUT_STREAM(is));

    while ((line = g_data_input_stream_read_line(dis,
                                                 NULL,
                                                 NULL,
                                                 error))) {
        const gchar *host;
        gchar *guest;
        GVirSandboxConfigMount *mnt = NULL;
        GList *mnts = NULL;
        gchar *tmp;

        guest = g_strdup(line);
        if ((tmp = strchr(guest, '=')) != NULL) {
            *tmp = '\0';
            host = tmp + 1;
        } else {
            host = guest;
        }

        mnts = priv->hostBindMounts;
        while (mnts) {
            mnt = GVIR_SANDBOX_CONFIG_MOUNT(mnts->data);
            const gchar *target = gvir_sandbox_config_mount_get_target(mnt);
            if (g_str_has_prefix(guest, target)) {
                guest = guest + strlen(target);
                break;
            }
            mnt = NULL;
            mnts = mnts->next;
        }
        if (!mnt) {
            g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                        "No mount with a prefix under %s", guest);
            g_free(guest);
            return FALSE;
        }

        gvir_sandbox_config_mount_add_include(mnt, host, guest);
        g_free(guest);
        g_free(line);
    }

    if (error && *error)
        goto cleanup;

    ret = TRUE;
cleanup:
    if (dis)
        g_object_unref(dis);
    g_object_unref(file);
    return ret;
}

/**
 * gvir_sandbox_config_get_memory:
 * @config: (transfer none): the sandbox config
 *
 * Retrieves the sandbox maximum available memory
 *
 * Returns: (transfer none): the current maximum available memory
 */
gulong gvir_sandbox_config_get_memory(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->memory;
}

/**
 * gvir_sandbox_config_set_security_label:
 * @config: (transfer none): the sandbox config
 * @label: (transfer none): the host directory
 *
 * Set the sandbox security label. By default a dynamic security label
 * is chosen. A static security label must be specified if any
 * custom mounts are added
 */
void gvir_sandbox_config_set_security_label(GVirSandboxConfig *config, const gchar *label)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    g_free(priv->secLabel);
    priv->secLabel = g_strdup(label);
}

/**
 * gvir_sandbox_config_get_security_label:
 * @config: (transfer none): the sandbox config
 *
 * Retrieve the sandbox security label
 *
 * Returns: (transfer none): the security label
 */
const gchar *gvir_sandbox_config_get_security_label(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->secLabel;
}


/**
 * gvir_sandbox_config_set_security_dynamic:
 * @config: (transfer none): the sandbox config
 * @dynamic: (transfer none): the security mode
 *
 * Set the SELinux security dynamic for the sandbox. The default
 * dynamic is "svirt_sandbox_t"
 */
void gvir_sandbox_config_set_security_dynamic(GVirSandboxConfig *config, gboolean dynamic)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    priv->secDynamic = dynamic;
}

/**
 * gvir_sandbox_config_get_security_dynamic:
 * @config: (transfer none): the sandbox config
 *
 * Retrieve the sandbox security mode
 *
 * Returns: (transfer none): the security mode
 */
gboolean gvir_sandbox_config_get_security_dynamic(GVirSandboxConfig *config)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    return priv->secDynamic;
}


gboolean gvir_sandbox_config_set_security_opts(GVirSandboxConfig *config,
                                               const gchar *optstr,
                                               GError **error)
{
    gchar **opts = g_strsplit(optstr, ",", 0);
    gsize i = 0;

    while (opts[i]) {
        gchar *name = opts[i];
        gchar *value = strchr(name, '=');

        if (strncmp(name, "label=", 5) == 0) {
            gvir_sandbox_config_set_security_label(config, value);
        } else if (g_str_equal(name, "dynamic")) {
            gvir_sandbox_config_set_security_dynamic(config, TRUE);
        } else if (g_str_equal(name, "static")) {
            gvir_sandbox_config_set_security_dynamic(config, FALSE);
        } else {
            g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                        "Unknown security option '%s'", name);
            return FALSE;
        }
    }
    return TRUE;
}


static GVirSandboxConfigMount *gvir_sandbox_config_load_config_mount(GKeyFile *file,
                                                                     const gchar *group,
                                                                     guint i,
                                                                     GError **error)
{
    GVirSandboxConfigMount *config = NULL;
    gchar *key = NULL;
    gchar *str = NULL;
    gchar *str2 = NULL;
    guint j;
    GError *e = NULL;

    key = g_strdup_printf("%s.%u", group, i);
    if ((str = g_key_file_get_string(file, key, "target", &e)) == NULL) {
        if (e->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            g_error_free(e);
            return NULL;
        }
        g_error_free(e);
        g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                    "%s", "Missing mount target in config file");
        g_free(str);
        goto error;
    }
    config = gvir_sandbox_config_mount_new(str);
    g_free(str);

    if ((str = g_key_file_get_string(file, key, "root", NULL)) == NULL) {
        g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                    "%s", "Missing mount root in config file");
        g_free(str);
        goto error;
    }
    gvir_sandbox_config_mount_set_root(config, str);
    g_free(str);

    for (j = 0 ; j < 1024 ; j++) {
        key = g_strdup_printf("%s.%u.include.%u", group, i, j);

        if ((str = g_key_file_get_string(file, key, "src", &e)) == NULL) {
            if (e->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
                g_error_free(e);
                e = NULL;
                break;
            }
            g_propagate_error(error, e);
            goto error;
        }

        str2 = g_key_file_get_string(file, key, "dst", NULL);
        gvir_sandbox_config_mount_add_include(config, str, str2);

        g_free(key);
    }


    return config;

error:
    g_free(key);
    g_object_unref(config);
    return NULL;
}


static GVirSandboxConfigNetwork *gvir_sandbox_config_load_config_network(GKeyFile *file,
                                                                         guint i,
                                                                         GError **error)
{
    GVirSandboxConfigNetwork *config = NULL;
    gchar *key = NULL;
    gchar *str1 = NULL;
    gchar *str2 = NULL;
    gboolean b;
    guint j;
    GError *e = NULL;

    key = g_strdup_printf("network.%u", i);
    b = g_key_file_get_boolean(file, key, "dhcp", &e);
    if (e) {
        if (e->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            g_error_free(e);
            return NULL;
        }
        g_error_free(e);
        e = NULL;
        b = TRUE;
    }
    config = gvir_sandbox_config_network_new();
    gvir_sandbox_config_network_set_dhcp(config, b);

    g_free(key);
    key = NULL;

    for (j = 0 ; j < 100 ; j++) {
        GInetAddress *primary = NULL;
        GInetAddress *broadcast = NULL;
        guint prefix;
        GVirSandboxConfigNetworkAddress *addr;
        key = g_strdup_printf("network.%u.address.%u", i, j);

        if ((str1 = g_key_file_get_string(file, key, "primary", &e)) == NULL) {
            if (e->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
                g_error_free(e);
                e = NULL;
                break;
            }
            g_propagate_error(error, e);
            goto error;
        }

        str2 = g_key_file_get_string(file, key, "broadcast", NULL);
        prefix = g_key_file_get_uint64(file, key, "prefix", NULL);

        primary = g_inet_address_new_from_string(str1);
        if (str2)
            broadcast = g_inet_address_new_from_string(str2);

        addr = gvir_sandbox_config_network_address_new(primary,
                                                       prefix,
                                                       broadcast);
        gvir_sandbox_config_network_add_address(config, addr);

        g_object_unref(primary);
        g_object_unref(broadcast);
        g_free(str1);
        g_free(str2);
        g_free(key);
        key = NULL;
    }


    for (j = 0 ; j < 100 ; j++) {
        guint prefix;
        GInetAddress *target = NULL;
        GInetAddress *gateway = NULL;
        GVirSandboxConfigNetworkRoute *route;
        key = g_strdup_printf("network.%u.route.%u", i, j);

        if ((str1 = g_key_file_get_string(file, key, "target", &e)) == NULL) {
            if (e->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
                g_error_free(e);
                e = NULL;
                break;
            }
            g_propagate_error(error, e);
            goto error;
        }

        str2 = g_key_file_get_string(file, key, "gateway", NULL);
        prefix = g_key_file_get_uint64(file, key, "prefix", NULL);

        target = g_inet_address_new_from_string(str1);
        if (str2)
            gateway = g_inet_address_new_from_string(str2);

        route = gvir_sandbox_config_network_route_new(target, prefix, gateway);
        gvir_sandbox_config_network_add_route(config, route);

        g_object_unref(target);
        g_object_unref(gateway);
        g_free(str1);
        g_free(str2);
        g_free(key);
        key = NULL;
    }


    return config;

error:
    g_free(key);
    g_object_unref(config);
    return NULL;
}


static gboolean gvir_sandbox_config_load_config(GVirSandboxConfig *config,
                                                GKeyFile *file,
                                                GError **error)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    gchar *str;
    gboolean b;
    guint64 u;
    guint i;
    GError *e = NULL;
    gboolean ret = FALSE;

    if ((str = g_key_file_get_string(file, "core", "name", NULL)) != NULL) {
        g_free(priv->name);
        priv->name = str;
    }
    if ((str = g_key_file_get_string(file, "core", "root", NULL)) != NULL) {
        g_free(priv->root);
        priv->root = str;
    }
    if ((str = g_key_file_get_string(file, "core", "arch", NULL)) != NULL) {
        g_free(priv->arch);
        priv->arch = str;
    }
    if ((str = g_key_file_get_string(file, "core", "kernrelease", NULL)) != NULL) {
        g_free(priv->kernrelease);
        priv->kernrelease = str;
    }
    if ((str = g_key_file_get_string(file, "core", "kernpath", NULL)) != NULL) {
        g_free(priv->kernpath);
        priv->kernpath = str;
    }
    b = g_key_file_get_boolean(file, "core", "shell", &e);
    if (e) {
        g_error_free(e);
        e = NULL;
    } else {
        priv->shell = b;
    }

    u = g_key_file_get_uint64(file, "identity", "uid", &e);
    if (e) {
        g_error_free(e);
        e = NULL;
    } else {
        priv->uid = u;
    }
    u = g_key_file_get_uint64(file, "identity", "gid", &e);
    if (e) {
        g_error_free(e);
        e = NULL;
    } else {
        priv->gid = u;
    }

    if ((str = g_key_file_get_string(file, "identity", "username", NULL)) != NULL) {
        g_free(priv->username);
        priv->username = str;
    }
    if ((str = g_key_file_get_string(file, "identity", "homedir", NULL)) != NULL) {
        g_free(priv->homedir);
        priv->homedir = str;
    }

    for (i = 0 ; i < 100 ; i++) {
        GVirSandboxConfigNetwork *network;
        if (!(network = gvir_sandbox_config_load_config_network(file, i, error)) &&
            *error)
            goto cleanup;
        if (network)
            priv->networks = g_list_append(priv->networks, network);
    }


    for (i = 0 ; i < 1024 ; i++) {
        GVirSandboxConfigMount *mount;
        if (!(mount = gvir_sandbox_config_load_config_mount(file, "hostbindmount", i, error)) &&
            *error)
            goto cleanup;
        if (mount)
            priv->hostBindMounts = g_list_append(priv->hostBindMounts, mount);
    }

    for (i = 0 ; i < 1024 ; i++) {
        GVirSandboxConfigMount *mount;
        if (!(mount = gvir_sandbox_config_load_config_mount(file, "guestbindmount", i, error)) &&
            *error)
            goto cleanup;
        if (mount)
            priv->guestBindMounts = g_list_append(priv->guestBindMounts, mount);
    }

    for (i = 0 ; i < 1024 ; i++) {
        GVirSandboxConfigMount *mount;
        if (!(mount = gvir_sandbox_config_load_config_mount(file, "hostimagemount", i, error)) &&
            *error)
            goto cleanup;
        if (mount)
            priv->hostImageMounts = g_list_append(priv->hostImageMounts, mount);
    }


    g_free(priv->secLabel);
    if ((str = g_key_file_get_string(file, "security", "label", NULL)) != NULL)
        priv->secLabel = str;
    else
        priv->secLabel = NULL;

    b = g_key_file_get_boolean(file, "security", "dynamic", &e);
    if (e) {
        g_error_free(e);
        e = NULL;
    } else {
        priv->secDynamic = b;
    }

    ret = TRUE;
cleanup:
    return ret;
}


static void gvir_sandbox_config_save_config_mount(GVirSandboxConfigMount *config,
                                                  GKeyFile *file,
                                                  const gchar *group,
                                                  guint i)
{
    gchar *key;
    GHashTable *includes;
    GHashTableIter iter;
    uint j;
    gpointer keyptr, valptr;

    key = g_strdup_printf("%s.%u", group, i);
    g_key_file_set_string(file, key, "target", gvir_sandbox_config_mount_get_target(config));
    g_key_file_set_string(file, key, "root", gvir_sandbox_config_mount_get_root(config));
    g_free(key);

    includes = gvir_sandbox_config_mount_get_includes(config);

    j = 0;
    g_hash_table_iter_init(&iter, includes);
    while (g_hash_table_iter_next(&iter, &keyptr, &valptr)) {
        const gchar *dst = keyptr;
        const gchar *src = valptr;

        key = g_strdup_printf("%s.%u.include.%u", group, i, j);
        g_key_file_set_string(file, key, "dst", dst);
        if (src)
            g_key_file_set_string(file, key, "src", src);
        g_free(key);

        j++;
    }
}


static void gvir_sandbox_config_save_config_network(GVirSandboxConfigNetwork *config,
                                                    GKeyFile *file,
                                                    guint i)
{
    gchar *key;
    uint j, k;
    GList *tmp, *addrs, *routes;

    j = 0;
    addrs = tmp = gvir_sandbox_config_network_get_addresses(config);
    while (tmp) {
        GVirSandboxConfigNetworkAddress *addr = tmp->data;
        GInetAddress *inet;
        gchar *str;

        key = g_strdup_printf("network.%u.address.%u", i, j);

        inet = gvir_sandbox_config_network_address_get_primary(addr);
        str = g_inet_address_to_string(inet);
        g_key_file_set_string(file, key, "primary", str);
        g_free(str);

        inet = gvir_sandbox_config_network_address_get_broadcast(addr);
        str = g_inet_address_to_string(inet);
        g_key_file_set_string(file, key, "broadcast", str);
        g_free(str);

        g_key_file_set_uint64(file, key, "prefix",
                              gvir_sandbox_config_network_address_get_prefix(addr));

        g_free(key);

        g_object_unref(addr);

        tmp = tmp->next;
        j++;
    }
    g_list_free(addrs);


    k = 0;
    routes = tmp = gvir_sandbox_config_network_get_routes(config);
    while (tmp) {
        GVirSandboxConfigNetworkRoute *route = tmp->data;
        GInetAddress *inet;
        gchar *str;
        guint64 prefix;

        key = g_strdup_printf("network.%u.route.%u", i, k);

        prefix = gvir_sandbox_config_network_route_get_prefix(route);
        g_key_file_set_uint64(file, key, "prefix", prefix);

        inet = gvir_sandbox_config_network_route_get_gateway(route);
        str = g_inet_address_to_string(inet);
        g_key_file_set_string(file, key, "gateway", str);
        g_free(str);

        inet = gvir_sandbox_config_network_route_get_target(route);
        str = g_inet_address_to_string(inet);
        g_key_file_set_string(file, key, "target", str);
        g_free(str);

        g_free(key);

        g_object_unref(route);

        tmp = tmp->next;
        k++;
    }
    g_list_free(routes);


    key = g_strdup_printf("network.%u", i);
    g_key_file_set_boolean(file, key, "dhcp", gvir_sandbox_config_network_get_dhcp(config));
    g_key_file_set_uint64(file, key, "addresses", j);
    g_key_file_set_uint64(file, key, "routes", k);
    g_free(key);
}



static void gvir_sandbox_config_save_config(GVirSandboxConfig *config,
                                            GKeyFile *file)
{
    GVirSandboxConfigPrivate *priv = config->priv;
    guint i;
    GList *tmp;

    g_key_file_set_string(file, "core", "name", priv->name);
    g_key_file_set_string(file, "core", "root", priv->root);
    g_key_file_set_string(file, "core", "arch", priv->arch);
    g_key_file_set_string(file, "core", "kernrelease", priv->kernrelease);
    g_key_file_set_string(file, "core", "kernpath", priv->kernpath);
    g_key_file_set_boolean(file, "core", "shell", priv->shell);

    g_key_file_set_uint64(file, "identity", "uid", priv->uid);
    g_key_file_set_uint64(file, "identity", "gid", priv->gid);
    g_key_file_set_string(file, "identity", "username", priv->username);
    g_key_file_set_string(file, "identity", "homedir", priv->homedir);

    i = 0;
    tmp = priv->hostBindMounts;
    while (tmp) {
        gvir_sandbox_config_save_config_mount(tmp->data,
                                              file,
                                              "hostbindmount",
                                              i);

        tmp = tmp->next;
        i++;
    }

    i = 0;
    tmp = priv->guestBindMounts;
    while (tmp) {
        gvir_sandbox_config_save_config_mount(tmp->data,
                                              file,
                                              "guestbindmount",
                                              i);

        tmp = tmp->next;
        i++;
    }

    i = 0;
    tmp = priv->hostImageMounts;
    while (tmp) {
        gvir_sandbox_config_save_config_mount(tmp->data,
                                              file,
                                              "hostimagemount",
                                              i);

        tmp = tmp->next;
        i++;
    }

    i = 0;
    tmp = priv->networks;
    while (tmp) {
        gvir_sandbox_config_save_config_network(tmp->data,
                                                file,
                                                i);

        tmp = tmp->next;
        i++;
    }

    if (priv->secLabel)
        g_key_file_set_string(file, "security", "label", priv->secLabel);
    g_key_file_set_boolean(file, "security", "dynamic", priv->secDynamic);
}


/**
 * gvir_sandbox_config_load_from_path:
 * @path: the local path to load
 * @error: the loader error
 *
 * Returns: (transfer full): the new config or NULL
 */
GVirSandboxConfig *gvir_sandbox_config_load_from_path(const gchar *path,
                                                      GError **error)
{
    GVirSandboxConfigClass *klass;
    GKeyFile *file = g_key_file_new();
    GVirSandboxConfig *config = NULL;
    gchar *str = NULL;
    GType type;

    /** XXX hack */
    GVIR_SANDBOX_TYPE_CONFIG_INTERACTIVE;
    GVIR_SANDBOX_TYPE_CONFIG_GRAPHICAL;
    GVIR_SANDBOX_TYPE_CONFIG_SERVICE;

    if (!g_key_file_load_from_file(file, path, G_KEY_FILE_NONE, error))
        goto cleanup;

    if ((str = g_key_file_get_string(file, "api", "class", NULL)) == NULL) {
        g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                    "%s", "Missing class name in config file");
        goto cleanup;
    }

    if (!(type = g_type_from_name(str))) {
        g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                    "Unknown type name '%s' in config file", str);
        goto cleanup;
    }

    if (!g_type_is_a(type,
                     GVIR_SANDBOX_TYPE_CONFIG)) {
        g_set_error(error, GVIR_SANDBOX_CONFIG_ERROR, 0,
                    "Type name '%s' in config file had wrong parent", str);
        goto cleanup;
    }

    config = g_object_new(type, NULL);

    klass = GVIR_SANDBOX_CONFIG_GET_CLASS(config);
    if (!(klass->load_config(config, file, error))) {
        g_object_unref(config);
        config = NULL;
        goto cleanup;
    }

cleanup:
    g_key_file_free(file);
    g_free(str);
    return config;
}


gboolean gvir_sandbox_config_save_to_path(GVirSandboxConfig *config,
                                          const gchar *path,
                                          GError **error)
{
    GVirSandboxConfigClass *klass = GVIR_SANDBOX_CONFIG_GET_CLASS(config);
    GKeyFile *file = g_key_file_new();
    gboolean ret = FALSE;
    gchar *data = NULL;
    GFile *f = g_file_new_for_path(path);
    GOutputStream *os = NULL;
    gsize len;

    g_key_file_set_string(file, "api", "class",
                          g_type_name(G_TYPE_FROM_CLASS(klass)));
    g_key_file_set_string(file, "api", "version",
                          VERSION);

    klass->save_config(config, file);

    if (!(data = g_key_file_to_data(file, &len, error)))
        goto cleanup;

    if (!(os = G_OUTPUT_STREAM(g_file_create(f, G_FILE_CREATE_PRIVATE, NULL, error))))
        goto cleanup;

    if (!g_output_stream_write_all(os, data, len, NULL, NULL, error))
        goto unlink;

    if (!g_output_stream_close(os, NULL, error))
        goto unlink;

    ret = TRUE;
cleanup:
    g_free(data);
    g_key_file_free(file);
    g_object_unref(f);
    if (os)
        g_object_unref(os);
    return ret;

unlink:
    g_file_delete(f, NULL, NULL);
    goto cleanup;
}

/*
 * libvirt-sandbox-builder-initrd.h: libvirt sandbox builder for virtual initrds
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

#if !defined(__LIBVIRT_SANDBOX_H__) && !defined(LIBVIRT_SANDBOX_BUILD)
#error "Only <libvirt-sandbox/libvirt-sandbox.h> can be included directly."
#endif

#ifndef __LIBVIRT_SANDBOX_BUILDER_INITRD_H__
#define __LIBVIRT_SANDBOX_BUILDER_INITRD_H__

G_BEGIN_DECLS

#define GVIR_SANDBOX_TYPE_BUILDER_INITRD            (gvir_sandbox_builder_initrd_get_type ())
#define GVIR_SANDBOX_BUILDER_INITRD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVIR_SANDBOX_TYPE_BUILDER_INITRD, GVirSandboxBuilderInitrd))
#define GVIR_SANDBOX_BUILDER_INITRD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GVIR_SANDBOX_TYPE_BUILDER_INITRD, GVirSandboxBuilderInitrdClass))
#define GVIR_SANDBOX_IS_BUILDER_INITRD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVIR_SANDBOX_TYPE_BUILDER_INITRD))
#define GVIR_SANDBOX_IS_BUILDER_INITRD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GVIR_SANDBOX_TYPE_BUILDER_INITRD))
#define GVIR_SANDBOX_BUILDER_INITRD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GVIR_SANDBOX_TYPE_BUILDER_INITRD, GVirSandboxBuilderInitrdClass))

typedef struct _GVirSandboxBuilderInitrd GVirSandboxBuilderInitrd;
typedef struct _GVirSandboxBuilderInitrdPrivate GVirSandboxBuilderInitrdPrivate;
typedef struct _GVirSandboxBuilderInitrdClass GVirSandboxBuilderInitrdClass;

struct _GVirSandboxBuilderInitrd
{
    GObject parent;

    GVirSandboxBuilderInitrdPrivate *priv;

    /* Do not add fields to this struct */
};

struct _GVirSandboxBuilderInitrdClass
{
    GObjectClass parent_class;

};

GType gvir_sandbox_builder_initrd_get_type(void);

GVirSandboxBuilderInitrd *gvir_sandbox_builder_initrd_new(void);

gboolean gvir_sandbox_builder_initrd_construct(GVirSandboxBuilderInitrd *builder,
                                               GVirSandboxConfigInitrd *config,
                                               gchar *outputfile,
                                               GError **error);

G_END_DECLS

#endif /* __LIBVIRT_SANDBOX_BUILDER_INITRD_H__ */

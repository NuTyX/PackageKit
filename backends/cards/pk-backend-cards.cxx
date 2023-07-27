/* pk-backend-cards.cpp
 *
 * Copyright (C) 2007-2014 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2023 Thierry Nuttens <tnut@nutyx.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>

#include <pk-backend.h>
#include <packagekit-glib2/pk-debug.h>

#include <libcards.h>

#include "job.h"

const gchar * pk_backend_get_description (PkBackend *backend)
{
	return "Cards backend";
}

const gchar * pk_backend_get_author (PkBackend *backend)
{
	return "Thierry Nuttens <tnut@nutyx.org>";
}

gboolean
pk_backend_supports_parallelization (PkBackend *backend)
{
	return FALSE;
}

void
pk_backend_initialize (GKeyFile *conf, PkBackend *backend)
{
	pk_debug_add_log_domain (G_LOG_DOMAIN);
	pk_debug_add_log_domain ("CARDS");
}

void
pk_backend_destroy (PkBackend *backend)
{
	g_debug("Cards backend being destroyed");
}

PkBitfield
pk_backend_get_groups (PkBackend *backend)
{
	return pk_bitfield_from_enums (PK_GROUP_ENUM_DOCUMENTATION,
		PK_GROUP_ENUM_DESKTOP_KDE,
		PK_GROUP_ENUM_DESKTOP_XFCE,
		PK_GROUP_ENUM_DESKTOP_GNOME,
		PK_GROUP_ENUM_DESKTOP_OTHER,
		-1);
}

PkBitfield
pk_backend_get_filters (PkBackend *backend)
{
	return pk_bitfield_from_enums (PK_FILTER_ENUM_BASENAME,
		PK_FILTER_ENUM_GUI,
		PK_FILTER_ENUM_APPLICATION,
		PK_FILTER_ENUM_INSTALLED,
		PK_FILTER_ENUM_NOT_INSTALLED,
		PK_FILTER_ENUM_DEVELOPMENT,
		PK_FILTER_ENUM_APPLICATION,
		-1);
}

gchar ** pk_backend_get_mime_types (PkBackend *backend)
{
	const gchar *mime_types[] = {
				"application/x-xz-compressed-tar",
				NULL };
	return g_strdupv ((gchar **) mime_types);
}

void
pk_backend_start_job (PkBackend *backend, PkBackendJob *job)
{

	/* create private state for this job */
	auto cards = new Job(job);

	/* you can use pk_backend_job_error_code() here too */
	pk_backend_job_set_user_data (job, cards);
}

void
pk_backend_stop_job (PkBackend *backend, PkBackendJob *job)
{
	auto cards = static_cast<Job*>(pk_backend_job_get_user_data (job));

	if (cards)
		delete cards;

	pk_backend_job_set_user_data (job, NULL);
}

void
pk_backend_cancel (PkBackend *backend, PkBackendJob *job)
{
	auto cards = static_cast<Job*>(pk_backend_job_get_user_data(job));
	if (cards) {
		/* try to cancel the thread */
		g_debug ("cancelling transaction");
		cards->cancel();
	}
}
static void
backend_refresh_cache_thread(PkBackendJob *job, GVariant *params, gpointer user_data)
{
	pk_backend_job_set_status(job, PK_STATUS_ENUM_REFRESH_CACHE);
	pk_backend_job_set_percentage (job, 0);
	Pkgsync Sync;
	Sync.run();
	pk_backend_job_set_percentage (job, 100);
	pk_backend_job_finished (job);
}
void
pk_backend_refresh_cache (PkBackend *backend, PkBackendJob *job, gboolean force)
{
	pk_backend_job_thread_create (job, backend_refresh_cache_thread, NULL, NULL);
}
static void
backend_get_updates_thread (PkBackendJob *job, GVariant *params, gpointer user_data)
{
	pk_backend_job_set_status (job, PK_STATUS_ENUM_QUERY);
	pk_backend_job_set_percentage (job, 0);
	Pkgsync Sync;
	Sync.run();
	pk_backend_job_set_percentage (job, 100);
	pk_backend_job_finished (job);
}
void
pk_backend_get_updates (PkBackend *backend, PkBackendJob *job, PkBitfield filters)
{
	pk_backend_job_thread_create (job, backend_get_updates_thread, NULL, NULL);
}

static void
backend_get_files_thread(PkBackendJob *job, GVariant *params, gpointer user_data)
{
	gchar **package_ids;	// List of packages
	gchar *pi;	// Package installed

	g_variant_get(params, "(^a&s)",
		&package_ids);

	auto cards = static_cast<Job*>(pk_backend_job_get_user_data(job));
	if (!cards->init()) {
		g_debug("Failed to create cards cache");
		return;
	}
	pk_backend_job_set_status(job, PK_STATUS_ENUM_QUERY);
	for (uint i = 0; i < g_strv_length(package_ids); ++i) {
		pi = package_ids[i];
		cards->emitPackageFiles(pi);
	}
}

void
pk_backend_get_files(PkBackend *backend, PkBackendJob *job, gchar **package_ids)
{
	pk_backend_job_thread_create(job, backend_get_files_thread, NULL, NULL);
}
static void
pk_backend_resolve_thread(PkBackendJob *job, GVariant *params, gpointer user_data)
{
	g_debug("pk_backend_resolve_tread started");

	gchar **search;
	PkBitfield filters;

	g_variant_get(params, "(t^a&s)",
		&filters,
		&search);
	pk_backend_job_set_allow_cancel(job, true);

	auto cards = static_cast<Job*>(pk_backend_job_get_user_data(job));
	if (!cards->init()) {
		g_debug("Failed to initialize CARDS job");
		return;
	}
	cards->resolvePackageIds(search);

	cards->emitPackages(filters,PK_INFO_ENUM_UNKNOWN, true);
}
void
pk_backend_resolve (PkBackend *backend, PkBackendJob *job, PkBitfield filters, gchar **package_ids)
{
	pk_backend_job_thread_create(job, pk_backend_resolve_thread, NULL, NULL);
}
void
pk_backend_install_packages (PkBackend *backend, PkBackendJob *job, PkBitfield transaction_flags, gchar **package_ids)
{
}

void
pk_backend_remove_packages (PkBackend *backend, PkBackendJob *job,
			    PkBitfield transaction_flags,
			    gchar **package_ids,
			    gboolean allow_deps,
			    gboolean autoremove)
{
}
PkBitfield
pk_backend_get_roles(PkBackend *backend)
{
	PkBitfield roles;
	roles = pk_bitfield_from_enums(
                PK_ROLE_ENUM_CANCEL,
                PK_ROLE_ENUM_GET_FILES,
                PK_ROLE_ENUM_RESOLVE,
                PK_ROLE_ENUM_REFRESH_CACHE,
                PK_ROLE_ENUM_GET_UPDATES,
                -1);

	return roles;
}

void pk_backend_search_details (PkBackend *backend, PkBackendJob *job, PkBitfield filters, gchar **values)
{
}

void pk_backend_search_files (PkBackend *backend, PkBackendJob *job, PkBitfield filters, gchar **values)
{
}

void pk_backend_search_groups (PkBackend *backend, PkBackendJob *job, PkBitfield filters, gchar **values)
{
}

void pk_backend_search_names (PkBackend *backend, PkBackendJob *job, PkBitfield filters, gchar **values)
{
}

void pk_backend_download_packages (PkBackend *backend, PkBackendJob *job, gchar **package_ids, const gchar *directory)
{
}
// vim:set ts=2 :

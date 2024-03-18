/* job.h
 *
 * Copyright (c) 2023 Thierry Nuttens <tnut@nutyx.org>
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
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#include <libcards.h>

#include <pk-backend.h>

class Job
{
public:
	Job(PkBackendJob *job);
	~Job();

	bool init();
	void cancel();
	bool cancelled() const;

	/**
	  * Returns the PackageKit backend job associated with this CARDS job.
	*/
	PkBackendJob *pkJob() const;

	/**
	  * Get the list of packages
	  *  generate m_packageSet, if the list is empty no package was found
	  */
	void resolvePackages(PkBitfield filters = PK_FILTER_ENUM_NONE);

	/**
	  * Tries to find a list of packages mathing the package ids
	  *  generate m_packageSet, if the list is empty no package was found
	  */
	void resolvePackageIds(gchar **package_ids, PkBitfield filters = PK_FILTER_ENUM_NONE);

	/**
	  * Return a package_id
	  *
	  */
	gchar* matchPackage(const std::string &package);

	/**
	  * Check if a given package matches the filters
	  *  @return true if it passed the filters
	  */
	bool matchPackage(const std::string &PackageName, PkBitfield filters);

	/**
	  * Refreshes the sources of packages
	  */
	void refreshCache();

	/**
	  *  Emits the files of a package
	  */
	void emitPackageFiles(const gchar *pi);

	/**
	  *  Emits the files of a package
	  */
	void emitPackageFilesLocal(const gchar *pi);


	/**
		* Emit m_packageSet that matches the given filters
	  */
	void emitPackages(PkBitfield filters = PK_FILTER_ENUM_NONE,
		PkInfoEnum state = PK_INFO_ENUM_UNKNOWN,
		bool multiversion = false);


private:
	Pkgdbh *m_cache;
	std::set<cards::Cache*> m_packageSet;
	PkBackendJob *m_job;
	bool	m_cancel;
	pid_t m_child_pid;
};
// vim:set ts=2 :

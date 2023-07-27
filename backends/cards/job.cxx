#include "job.h"

#include <iostream>
#include <sstream>
#include <memory>
#include <fstream>
#include <dirent.h>

Job::Job(PkBackendJob *job) :
	m_cache(nullptr),
	m_job(job),
	m_cancel(false)
{
}
Job::~Job()
{
	delete m_cache;
}
bool Job::init()
{
	m_cache = new Pkgdbh;

	if (m_cache->getListOfPackagesNames().size() > 0)
		return true;

	return false;
}
void Job::cancel()
{
    if (!m_cancel) {
        m_cancel = true;
        pk_backend_job_set_status(m_job, PK_STATUS_ENUM_CANCEL);
    }

    if (m_child_pid > 0) {
        kill(m_child_pid, SIGTERM);
    }
}

bool Job::cancelled() const
{
	return m_cancel;
}

PkBackendJob *Job::pkJob() const
{
	return m_job;
}

void Job::resolvePackageIds(gchar **package_ids, PkBitfield filters)
{
	Pkg* pkg = new Pkg;
	g_debug("resolvePackgesIds");

	pk_backend_job_set_status (m_job, PK_STATUS_ENUM_QUERY);

	if (package_ids == NULL)
		return;

	for (uint i = 0; i < g_strv_length(package_ids); ++i) {
		if (m_cancel)
			break;
		std::string pi = package_ids[i];
		for (auto p: m_cache->getListOfPackagesNames()) {
			if (p != pi)
				continue;
			pkg->setName(p);
			pkg->setDescription(m_cache->getDescription(p));
			pkg->setArch(m_cache->getArch(p));
			pkg->setVersion(m_cache->getVersion(p));
			pkg->setCollection(m_cache->getCollection(p));
			m_packageSet.insert(pkg);
		}
	}

}

gchar* Job::matchPackage(const std::string &package)
{
  const gchar *name, *version, *arch, *collection;

  name = package.c_str();
  version = m_cache->getVersion(package).c_str();
  arch = m_cache->getArch(package).c_str();
  collection = m_cache->getCollection(package).c_str();

  return pk_package_id_build (name, version, arch, collection);

}
bool Job::matchPackage(const std::string& PackageName, PkBitfield filters)
{
	return true;
}
void Job::refreshCache()
{
	pk_backend_job_set_status(m_job, PK_STATUS_ENUM_REFRESH_CACHE);
}
void Job::emitPackageFiles(const gchar *pi)
{
	using namespace std;
	GPtrArray *files;
	string line;

	g_auto(GStrv) parts = pk_package_id_split(pi);
	string fName;
	fName = "/var/lib/pkg/DB/" + string(parts[PK_PACKAGE_ID_NAME]) + "/files";

	if (checkFileExist(fName)) {
		ifstream in(fName.c_str());
		if (!in != 0) {
			return;
		}

		files = g_ptr_array_new_with_free_func(g_free);
		while (in.eof() == false) {
			getline(in, line);
			if (!line.empty()) {
				g_ptr_array_add(files, g_strdup(line.c_str()));
			}
		}

		if (files->len) {
			g_ptr_array_add(files, NULL);
			pk_backend_job_files(m_job, pi, (gchar **) files->pdata);
		}
		g_ptr_array_unref(files);
	}
}
void Job::emitPackages(PkBitfield filters, PkInfoEnum state, bool multiversion)
{
	for (auto p:m_packageSet) {
		if(m_cancel)
			break;

		std::string d = p->getDescription();
		std::string v = p->getVersion();
		std::string a = p->getArch();
		std::string c = p->getCollection();
		std::string pi = p->getName() + ";" + v + ";" + a + ";" + c;
		pk_backend_job_package(m_job, PK_INFO_ENUM_INSTALLED, pi.c_str(), d.c_str());
	}
}
// vim:set ts=2 :
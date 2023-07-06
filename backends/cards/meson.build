add_languages('cpp')

cpp_compiler = meson.get_compiler('cpp')

archive_dep = meson.get_compiler('c').find_library('archive')
curl_dep = meson.get_compiler('c').find_library('curl')
cards_dep = meson.get_compiler('c').find_library('cards')

shared_module(
  'pk_backend_cards',
  'pk-backend-cards.cxx',
  'job.cxx',
  'job.h',
  include_directories: packagekit_src_include,
  dependencies: [
    packagekit_glib2_dep,
    archive_dep,
    curl_dep,
    cards_dep,
    gmodule_dep,
  ],
  cpp_args: [
    '-DPK_COMPILATION',
    '-DG_LOG_DOMAIN="PackageKit-Cards"',
  ],
  override_options: ['c_std=c11','cpp_std=c++17'],
  install: true,
  install_dir: pk_plugin_dir,
)
